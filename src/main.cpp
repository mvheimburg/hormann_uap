/*
 * Hoermann Garagen-Tor mit ESP8266 und UAP1
 *
 * features:
 * mqtt
 * support for
 * S4 Zu/close
 * S2 Auf/open
 *
 * GNU General Public License v2.0
 *
*/
#include <stdio.h>
#include <string.h>
#include <Ethernet3.h>
// #include <ICMPPing.h>

// Uses Tasker to do a nonblocking loop every 1 second to check gate_state
#include "Tasker.h"
#include <PubSubClient.h>

#include "secrets.h"

// MQTT Gate commands, topic and paylaods
// #define MQTT_GATE_TOPIC_CONFIG "homeassistant/cover/garage/config"



#define MQTT_GATE_TOPIC_CMD "garage/gate/cmd"
#define PAYLOAD_CMD_GATE_OPEN "OPEN"
#define PAYLOAD_CMD_GATE_CLOSE "CLOSE"
#define PAYLOAD_CMD_GATE_STOP "STOP"

// MQTT Gate state feedback, topic and paylaods
#define MQTT_GATE_TOPIC_STATUS "garage/gate/state"
#define PAYLOAD_GATE_OPENED "open"
#define PAYLOAD_GATE_OPENING "opening"
#define PAYLOAD_GATE_CLOSED "closed"
#define PAYLOAD_GATE_CLOSING "closing"
// Gate states --- must match payload. Limited due to limitations in Home Assistant MQTT Cover. Might be extended in future.
const char* gate_state_payloads[] = { PAYLOAD_GATE_OPENED, PAYLOAD_GATE_OPENING, PAYLOAD_GATE_CLOSED, PAYLOAD_GATE_CLOSING };
enum gate_states { GATE_OPEN, GATE_OPENING, GATE_CLOSED, GATE_CLOSING };
// MQTT Light commands, topic and paylaods
#define MQTT_LIGHT_TOPIC_CMD "garage/light/cmd"
#define PAYLOAD_CMD_LIGHT_ON "ON"
#define PAYLOAD_CMD_LIGHT_OFF "OFF"

// MQTT Light state feedback, topic and paylaods
#define MQTT_LIGHT_TOPIC_STATUS "garage/light/state"
#define PAYLOAD_LIGHT_ON "on"
#define PAYLOAD_LIGHT_OFF "off"
const char* light_state_payloads[] = { PAYLOAD_LIGHT_ON, PAYLOAD_LIGHT_OFF };
enum light_states { LIGHT_ON, LIGHT_OFF };

// Please update mac and IP address according to your local network
#if defined(WIZ550io_WITH_MACADDRESS) // Use assigned MAC address of WIZ550io
;
#else
uint8_t mac[] = MAC;
#endif
IPAddress broker = BROKER;

// EthernetClient client;
EthernetClient ethClient;
PubSubClient client(ethClient);

// int gpio_led = 2; // internal blue LED
#define SWITCH_DELAY 500
int gpio_state_gate_open = 2;
int gpio_state_gate_closed = 3;
int gpio_state_light_on = 4;


int gpio_command_gate_open = A0;
int gpio_command_gate_close = A1;
int gpio_command_gate_stop = A2;
int gpio_command_light_on = A3;


int gate_state = NULL;
int light_state = NULL;
const char* last_command = NULL;
unsigned long force_state_update_timer = 60000;
bool gate_moving = false;
// bool gate_state_changed = false;
// bool light_state_changed = false;
// bool force_state_update = false;

Tasker tasker;

// STATES SHOULD BE: OPEN, OPENING, CLOSED, CLOSING, ERROR

//check gpio (input of hoermann uap1)
void publish_gate_state() {
  Serial.println("mqtt gate_state update: ");
  Serial.println(gate_state_payloads[gate_state]);
  client.publish(MQTT_GATE_TOPIC_STATUS, gate_state_payloads[gate_state]);
  // gate_state_changed = false;
}

void check_gate_state() {
  Serial.println("Checking gate state");
  int prev_gate_state = gate_state;

  if ((digitalRead(gpio_state_gate_open) == LOW) && (digitalRead(gpio_state_gate_closed) == HIGH  )) {
    gate_state = GATE_OPEN;
    gate_moving = false;
    }
  else if ((digitalRead(gpio_state_gate_open) == HIGH  ) && (digitalRead(gpio_state_gate_closed) == LOW)) {
    gate_state = GATE_CLOSED;
    gate_moving = false;
    }
  else if ((digitalRead(gpio_state_gate_open) == LOW) && (digitalRead(gpio_state_gate_closed) == LOW)) {
    if (gate_moving == false) {
      if (last_command == PAYLOAD_CMD_GATE_OPEN) {
        gate_state = GATE_OPENING;
        gate_moving = true;
      }
      else if (last_command == PAYLOAD_CMD_GATE_CLOSE) {
        gate_state = GATE_CLOSING;
        gate_moving = true;
      }
    }
    else {
      if (last_command == PAYLOAD_CMD_GATE_STOP) {
         gate_state = GATE_OPEN;
         gate_moving = false;
      }
    }

  }

  if ( prev_gate_state != gate_state ) {
    Serial.println("dbg: Status has changed: ");
    // gate_state_changed = true;
    publish_gate_state();

  }
}


void publish_light_state() {
  Serial.println("mqtt light_state update: ");
  Serial.println(light_state_payloads[light_state]);
  client.publish(MQTT_LIGHT_TOPIC_STATUS, light_state_payloads[light_state]);
    // light_state_changed = false;
}

void check_light_state() {
  int prev_light_state = light_state;
  Serial.println("Checking light state");

  if (digitalRead(gpio_state_light_on) == HIGH) {
    light_state = LIGHT_ON;
  }
  else {
    light_state = LIGHT_OFF;
  }
  if ( prev_light_state != light_state )
  {
    Serial.println("dbg: Status has changed: ");
    // light_state_changed = true;
    publish_light_state();
  }
}



void trigger_gate(int pin) {
  Serial.println("writing to pin");
  Serial.println(pin);
  digitalWrite(pin, LOW);
  delay(SWITCH_DELAY);
  digitalWrite(pin, HIGH);
}


void open_gate() {
  Serial.println("open_gate starting");
  trigger_gate(gpio_command_gate_open);
}


void close_gate() {
  Serial.println("close_gate starting");
  trigger_gate(gpio_command_gate_close);
}

void stop_gate() {
  Serial.println("stop_gate starting");
  trigger_gate(gpio_command_gate_stop);
}

void set_light(bool on) {
  if (on) {

    Serial.println("Turning light on");
    digitalWrite(gpio_command_light_on, HIGH);
  }
  else {
    Serial.println("Turning light off");
    digitalWrite(gpio_command_light_on, LOW);
  }
}



void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = 0;

  Serial.println("mqtt call back");
  Serial.println( topic );
  Serial.println(  (const char *) payload );
  if (strstr( MQTT_GATE_TOPIC_CMD, topic) != NULL) {
    if ( strstr( (const char *)payload, PAYLOAD_CMD_GATE_OPEN) != NULL) {
      open_gate();
    }
    else if ( strstr( (const char *)payload, PAYLOAD_CMD_GATE_CLOSE) != NULL) {
      close_gate();
    }
    else if ( strstr( (const char *)payload, PAYLOAD_CMD_GATE_STOP) != NULL) {
      stop_gate();
    }
  }
  else if (strstr( MQTT_LIGHT_TOPIC_CMD, topic) != NULL) {
    if ( strstr( (const char *)payload, PAYLOAD_CMD_LIGHT_ON) != NULL) {
      set_light(true);
    }
    else if ( strstr( (const char *)payload, PAYLOAD_CMD_LIGHT_OFF) != NULL) {
      set_light(false);
    }
  }
}


void mqtt_reconnect() {
  while (!client.connected()) {
    Serial.println(Ethernet.linkReport());
    Serial.print("Connect to MQTT Broker ");
    Serial.println( broker );
    delay(1000);
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PWD)) {
      Serial.print("connected: topic ");
      Serial.println( MQTT_GATE_TOPIC_CMD  );
      client.subscribe(MQTT_GATE_TOPIC_CMD);
      Serial.println( MQTT_LIGHT_TOPIC_CMD  );
      client.subscribe(MQTT_LIGHT_TOPIC_CMD);
    } else {
      Serial.print("failed: ");
      Serial.print(client.state());
      Serial.println(" try again...");
      delay(5000);
    }
  }
  Serial.println(" ok...");
}


void setup(void) {
  Ethernet.setHostname(MQTT_CLIENT_ID);
  Ethernet.begin(mac);

  //Your MQTT Broker
  Serial.begin(9600);
  delay(1000);

  Serial.println("\n\Booting hoermann garage controller");
  Serial.println();

  pinMode(gpio_state_gate_open, INPUT_PULLUP);
  pinMode(gpio_state_gate_closed, INPUT_PULLUP);
  pinMode(gpio_state_light_on, INPUT_PULLUP);


  pinMode(gpio_command_gate_close, OUTPUT);
  digitalWrite(gpio_command_gate_close, HIGH);
  pinMode(gpio_command_light_on, OUTPUT);
  digitalWrite(gpio_command_light_on, HIGH);
  pinMode(gpio_command_gate_open, OUTPUT);
  digitalWrite(gpio_command_gate_open, HIGH);
  pinMode(gpio_command_gate_stop, OUTPUT);
  digitalWrite(gpio_command_gate_stop, HIGH);



  client.setServer(broker, BROKER_PORT);
  client.setCallback(mqtt_callback);
  mqtt_reconnect();


  // wait for stable UAP1 outputs after power OFF
  for ( int iLoop=0; iLoop<12; iLoop++) {
    // digitalWrite(gpio_led, HIGH );
    check_gate_state();
    check_light_state();
    delay(250);
  }

  //Check the gate gate_state every 1 second
  tasker.setInterval(check_gate_state, 1000);
  tasker.setInterval(check_light_state, 1000);
}


void loop(void) {

  if (!client.connected()) {
    mqtt_reconnect();
  }
  client.loop();
  tasker.loop();

}

