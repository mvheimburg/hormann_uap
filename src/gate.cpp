
// gate.cpp
#include "gate.h"
#include "secrets.h"



Gate::Gate(PubSubClient* client) {
    this->client = client;

    pinMode(this->gpio_state_gate_open, INPUT_PULLUP);
    pinMode(this->gpio_state_gate_closed, INPUT_PULLUP);
    pinMode(this->gpio_state_light_on, INPUT_PULLUP);

    pinMode(this->gpio_command_gate_close, OUTPUT);
    digitalWrite(this->gpio_command_gate_close, HIGH);
    pinMode(this->gpio_command_light_on, OUTPUT);
    digitalWrite(this->gpio_command_light_on, HIGH);
    pinMode(this->gpio_command_gate_open, OUTPUT);
    digitalWrite(this->gpio_command_gate_open, HIGH);
    pinMode(this->gpio_command_gate_stop, OUTPUT);
    digitalWrite(this->gpio_command_gate_stop, HIGH);
}

pin_states Gate::pinState() {
	if ((digitalRead(gpio_state_gate_open) == LOW) && (digitalRead(gpio_state_gate_closed) == HIGH  )) {
    return PIN_OPEN;
    }
  else if ((digitalRead(gpio_state_gate_open) == HIGH  ) && (digitalRead(gpio_state_gate_closed) == LOW)) {
    return PIN_CLOSED;
    }
	return PIN_BETWEEN;
}
	

void Gate::checkState() {
  Serial.println("Checking gate this->state");
  int prev_state = this->state;
	int pin_state = this->pinState();

	if ( pin_state == PIN_OPEN and this->state != GATE_OPEN ){
		this->state = GATE_OPEN;
		this->position = GATE_POS_OPEN;
	}
	else if ( pin_state == PIN_CLOSED and this->state != GATE_CLOSED ){
		this->state = GATE_CLOSED;
		this->position = GATE_POS_CLOSED;
	}
	else if ( pin_state == PIN_BETWEEN and this->state == GATE_CLOSED ){
		this->state = GATE_CLOSING;
		this->last_move_t = millis();
	}
	else if ( pin_state == PIN_BETWEEN and this->state == GATE_OPEN ){
		this->state = GATE_OPENING;
		this->last_move_t = millis();
	}

  if ( pin_state == PIN_BETWEEN ) {
		long now = millis();
		long delta = this->last_move_t - now;
		if (this->state == GATE_OPENING){
			this->position += round((delta / GATE_SPD_UP ) * 100);
		} else if (this->state == GATE_CLOSING){
			this->position -= round((delta / GATE_SPD_DWN ) * 100);
		}
		this->publishPos();
	}
  if (prev_state != this->state) {
    this->publishState();
  }
}

void Gate::publishState() {
  Serial.println("mqtt gate_state update: ");
  Serial.println(gate_state_payloads[this->state]);
  this->client->publish(MQTT_GATE_TOPIC_STATUS, gate_state_payloads[this->state]);
}

void Gate::publishPos() {
	Serial.println("mqtt gate_pos update: ");
	char pos[8];
	itoa(this->position, pos, 10);
  this->client->publish(MQTT_GATE_TOPIC_POSITION, pos);
}

void Gate::checkLightState() {
  int prev_light_state = this->light;
  Serial.println("Checking light state");

  if (digitalRead(this->gpio_state_light_on) == HIGH) {
    this->light = LIGHT_ON;
  }
  else {
    this->light = LIGHT_OFF;
  }
  if ( prev_light_state != this->light ) {
    // Serial.println("dbg: Status has changed: ");
    // light_state_changed = true;
    return this->publishLightState();
  } 
}

void Gate::publishLightState() {
  Serial.println("mqtt light update: ");
  Serial.println(light_state_payloads[this->light]);
  client->publish(MQTT_LIGHT_TOPIC_STATUS, light_state_payloads[this->light]);
}


void Gate::triggerGate(int pin) {
  Serial.println("writing to pin");
  Serial.println(pin);
  digitalWrite(pin, LOW);
  delay(SWITCH_DELAY);
  digitalWrite(pin, HIGH);
}


void Gate::openGate() {
  Serial.println("open_gate starting");
  this->triggerGate(this->gpio_command_gate_open);
	if ( this->pinState() == PIN_BETWEEN ){
		this->state = GATE_OPENING;
		this->publishState();
		this->last_move_t  = millis();
	}
}


void Gate::closeGate() {
  Serial.println("close_gate starting");
  this->triggerGate(this->gpio_command_gate_close);
	if ( this->pinState() == PIN_BETWEEN ){
		this->state = GATE_CLOSING;
		this->publishState();
		this->last_move_t  = millis();
	}
}

void Gate::stopGate() {
  Serial.println("stop_gate starting");
  this->triggerGate(this->gpio_command_gate_stop);
	if ( this->pinState() == PIN_BETWEEN ){
		this->state = GATE_STOPPED;
		this->last_move_t  = millis();
	}
}

void Gate::triggerLight(bool on) {
  if (on) {

    Serial.println("Turning light on");
    digitalWrite(this->gpio_command_light_on, HIGH);
  }
  else {
    Serial.println("Turning light off");
    digitalWrite(this->gpio_command_light_on, LOW);
  }
}


void Gate::activate(char* topic, byte* payload, unsigned int length) {
  payload[length] = 0;

  Serial.println("mqtt call back");
  Serial.println( topic );
  Serial.println(  (const char *) payload );
  if (strstr( MQTT_GATE_TOPIC_CMD, topic) != NULL) {
    if ( strstr( (const char *)payload, PAYLOAD_CMD_GATE_OPEN) != NULL) {
      this->openGate();
    }
    else if ( strstr( (const char *)payload, PAYLOAD_CMD_GATE_CLOSE) != NULL) {
      this->closeGate();
    }
    else if ( strstr( (const char *)payload, PAYLOAD_CMD_GATE_STOP) != NULL) {
      this->stopGate();
    }
  }
  else if (strstr( MQTT_LIGHT_TOPIC_CMD, topic) != NULL) {
    if ( strstr( (const char *)payload, PAYLOAD_CMD_LIGHT_ON) != NULL) {
      this->triggerLight(true);
    }
    else if ( strstr( (const char *)payload, PAYLOAD_CMD_LIGHT_OFF) != NULL) {
      this->triggerLight(false);
    }
  }
}

void Gate::subscribe(){
    Serial.print("subscribing \n");
    Serial.println( MQTT_GATE_TOPIC_CMD  );
		Serial.print("\n");
    this->client->subscribe(MQTT_GATE_TOPIC_CMD);
    Serial.println( MQTT_LIGHT_TOPIC_CMD  );
		Serial.print("\n");
    this->client->subscribe(MQTT_LIGHT_TOPIC_CMD);
}

    

// bool Gate::mqttReconnect() {
//   if (!this->client->connected()) {
//     Serial.print("Connecting to MQTT Broker");
//     delay(1000);
//     if (this->client->connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PWD)) {
//         Serial.print("connected: now subscribing");
//         Serial.println( MQTT_GATE_TOPIC_CMD  );
//         client->subscribe(MQTT_GATE_TOPIC_CMD);
//         Serial.println( MQTT_LIGHT_TOPIC_CMD  );
//         client->subscribe(MQTT_LIGHT_TOPIC_CMD);
//     } else {
//         Serial.print("failed: ");
//         Serial.print(client->state());
//         Serial.println(" try again...");
//         delay(5000);
//     }
//   }
//   return this->client->connected();
// }


// void Gate::setup(){
//     this->last_reconnect_attempt = 0;

//     Serial.println("\n Booting hoermann garage controller \n");
//     for ( int iLoop=0; iLoop<12; iLoop++) {
//         this->checkState();
//         this->checkLightState();
//         delay(250);
//     }

    // this->client->setServer(broker, BROKER_PORT);
    // this->client->setCallback(this->mqttCallback);

    // Task stateTask(200, TASK_FOREVER, this->checkState, this->tasker, true);  //adding task to the chain on creation
    // Task lightTask(1000, TASK_FOREVER, this->checkLightState, this->tasker, true);  //adding task to the chain on creation
    // this->tasker.startNow();
    // this->tasker.setInterval(this->checkState, 200);
    // this->tasker.setInterval(this->checkLightState, 1000);
// }


// void Gate::loop(void){
//     this->client->loop();
//     // this->tasker.loop();
//     // this->tasker.execute();
// }


// void Gate::reconnect(void){
//     long now = millis();
//     if (now - this->last_reconnect_attempt > 5000) {
//         this->last_reconnect_attempt = now;
//         // Attempt to reconnect
//         if (this->mqttReconnect()) {
//         this->last_reconnect_attempt = 0;
//         }
//     }
// }

// bool Gate::connected(void){
//     return this->client->connected();
// }
