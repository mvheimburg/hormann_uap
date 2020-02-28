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
#include <Ethernet.h>

// Uses Ticker to do a nonblocking loop every 1 second to check status
#include <Ticker.h>
#include <PubSubClient.h>

unsigned long lastmillis;

const char* sVersion = "0.8.24";

#define CLOSED LOW
#define OPEN HIGH

//Your MQTT Broker
const char* mqtt_server = "hwr-pi";

// MQTT client is also host name for WIFI
const char* mqtt_client_id = "garagentor";

const char* mqtt_topic_cmd = "garage/gate/cmd";
const char* mqtt_topic_status = "garage/gate/status";
const char* mqtt_topic_action = "tuer/garagentor/action";


byte mac[]    = {  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };  // Ethernet shield (W5100) MAC address
IPAddress ip(192, 168, 2, 105);                           // Ethernet shield (W5100) IP address


EthernetClient ethClient;
PubSubClient client(ethClient);


int gpioLed = 2; // internal blue LED

int gpioO1TorOben = 5;  // D1
int gpioO2TorUnten = 0; // D3

int gpioS4CmdZu = 14;          // D5
int gpioS5CmdLampeToggle = 12; // D6
int gpioS2CmdAuf = 13;         // D7
int gpioS3CmdHalb = 15;        // D8


int GATE_ERROR=0;
int GATE_CLOSED=1;
int GATE_OPEN=2;
int GATE_HALF_OPEN=3;

const char* status_message[] = { "Error", "Closed", "Open", "Half open" };

int status = GATE_ERROR;

bool SendUpdate = false;

Ticker ticker;


// STATES SHOULD BE: OPEN, OPENING, CLOSED, CLOSING, ERROR

//check gpio (input of hoermann uap1)
void status_gate() {
  int myOldStatus = status;
 
  if ((digitalRead(gpioO1TorOben) == OPEN  ) && (digitalRead(gpioO2TorUnten) == OPEN  )) { status = GATE_HALF_OPEN; }
  if ((digitalRead(gpioO1TorOben) == CLOSED) && (digitalRead(gpioO2TorUnten) == OPEN  )) { status = GATE_OPEN; }
  if ((digitalRead(gpioO1TorOben) == OPEN  ) && (digitalRead(gpioO2TorUnten) == CLOSED)) { status = GATE_CLOSED; }
  if ((digitalRead(gpioO1TorOben) == CLOSED) && (digitalRead(gpioO2TorUnten) == CLOSED)) { status = GATE_ERROR; }

  if ( myOldStatus != status )
  {
    Serial.print("dbg: Status has changed: ");
    Serial.println(status_message[status]);
  }
}



void open_gate() {
      Serial.println("open_gate starting");
      client.publish(mqtt_topic_status, "opening");

      digitalWrite(gpioS2CmdAuf, HIGH);
      delay(1000);
      digitalWrite(gpioS2CmdAuf, LOW);

      Serial.println("open_gate finished");
      client.publish(mqtt_topic_status, "open");
}


void close_gate() {
      Serial.println("close_gate starting");
      client.publish(mqtt_topic_status, "closing");

      digitalWrite(gpioS4CmdZu, HIGH);
      delay(1000);
      digitalWrite(gpioS4CmdZu, LOW);

      Serial.println("close_gate finished");
      client.publish(mqtt_topic_status, "closed");
}



void MqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = 0;

  Serial.println("mqtt call back");
  Serial.println( topic );
  Serial.println(  (const char *) payload );
  
  if ( strstr( (const char *)payload, "zu") != NULL) {
    close_gate();
  }
  if ( strstr( (const char *)payload, "auf") != NULL) {
    open_gate();
  }
  if ( strstr( (const char *)payload, "version") != NULL) {
    client.publish(mqtt_topic_version, sVersion);
    long lRssi = WiFi.RSSI();
    char sBuffer[50];
    sprintf(sBuffer, "RSSI = %d dBm", lRssi);
    client.publish(mqtt_topic_wifi, sBuffer);
  }
  if ( strstr( (const char *)payload, "status") != NULL) {
    status_gate();
    client.publish(mqtt_topic_status, status_message[status]);
  }
}


void MqttReconnect() {
  while (!client.connected()) {
    Serial.print("Connect to MQTT Broker "); 
    Serial.println( mqtt_server );
    delay(1000);
    if (client.connect(mqtt_client_id)) {
      Serial.print("connected: topic ");  
      Serial.println( mqtt_topic_cmd );
      client.subscribe(mqtt_topic_cmd);
    } else {
      Serial.print("failed: ");
      Serial.print(client.state());
      Serial.println(" try again...");
      delay(5000);
    }
  }
  Serial.println(" ok...");
}



void CheckDoorStatus() {
  int oldStatus = status;
  status_gate();
  if (oldStatus == status)
  {
    //new status is the same as the current status, return
    return;
  }
  else
  {
    Serial.print("Status has changed to: ");
    Serial.println(status_message[status]);
    SendUpdate = true;
    digitalWrite(gpioLed, (status != GATE_CLOSED) ); // LED on bei Fehler  
    delay(1000);
    digitalWrite(gpioLed, (status == GATE_CLOSED) ); // LED on bei Fehler
  }
}


void setup(void) {

  Serial.begin(115200);
  delay(300);
  Serial.println("\n\nstarte hoermann garage...");
  Serial.println();
  Serial.println(sVersion);
  Serial.println();

  lastmillis = millis();

  pinMode(gpioO1TorOben, INPUT_PULLUP);
  pinMode(gpioO2TorUnten, INPUT_PULLUP);

  pinMode(gpioLed, OUTPUT);

  pinMode(gpioS4CmdZu, OUTPUT);
  digitalWrite(gpioS4CmdZu, LOW);
  pinMode(gpioS5CmdLampeToggle, OUTPUT);
  digitalWrite(gpioS5CmdLampeToggle, LOW);
  pinMode(gpioS2CmdAuf, OUTPUT);
  digitalWrite(gpioS2CmdAuf, LOW);
  pinMode(gpioS3CmdHalb, OUTPUT);
  digitalWrite(gpioS3CmdHalb, LOW);



  client.setServer(mqtt_server, 1883);
  client.setCallback(MqttCallback);
  MqttReconnect();
  client.publish(mqtt_topic_version, sVersion);



  // warte auf stabile UAP1-Ausgänge nach Strom-AUS
  for ( int iLoop=0; iLoop<12; iLoop++) {
    digitalWrite(gpioLed, HIGH );
    status_gate();
    delay(250);
    digitalWrite(gpioLed, LOW );
    status_gate();
    delay(250);
  }

  //Check the door status every 1 second
  ticker.attach(1, CheckDoorStatus);

}



void loop(void) {

  if (!client.connected()) { MqttReconnect(); }
  client.loop();

  if (millis() - lastmillis >  60000) {
    Serial.print("still running since ");
    Serial.print(millis()/60000);
    Serial.println(" minutes");
    lastmillis = millis();
    SendUpdate = true;
  }

  if (SendUpdate)
  {
    Serial.print("mqtt status update: ");
    Serial.println(status_message[status]);
    client.publish(mqtt_topic_status, status_message[status]);
    SendUpdate = false;
  }

}

