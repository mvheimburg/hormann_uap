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
// #include <TaskScheduler.h>
#include <Tasker.h>

#include "gate.h"
#include "secrets.h"
#include "mqtt_specs.h"



// Please update mac and IP address according to your local network
#if defined(WIZ550io_WITH_MACADDRESS) // Use assigned MAC address of WIZ550io
;
#else
uint8_t mac[] = MAC;
#endif

int last_reconnect_attempt = 0;

// EthernetClient client;
IPAddress broker = BROKER;
EthernetClient ethClient;
PubSubClient client(ethClient);
Gate gate(&client);
Tasker tasker;


void mqttCallback(char* topic, byte* payload, unsigned int length){
	Serial.println("payload received on topic \n");
	Serial.println(topic);
	Serial.println("/n");
  gate.activate(topic, payload, length);
}

void checkState(){
  gate.checkState();
}

void checkLightState(){
  gate.checkLightState();
}

void setup(void) {
  Serial.begin(9600);
  delay(200);
  Serial.println("\n Booting hoermann garage controller \n");
  Ethernet.begin(mac);
  // gate.setup()

  last_reconnect_attempt = 0;

  client.setServer(broker, BROKER_PORT);
	client.setCallback(mqttCallback);
	tasker.setInterval(checkState, 500);
  tasker.setInterval(checkLightState, 1000);
  delay(1500);
}


bool mqttReconnect() {
  if (!client.connected()) {
    Serial.print("Connecting to MQTT Broker \n");
    delay(1000);
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PWD)) {
      gate.subscribe();
    } else {
			Serial.print("failed: \n");
			Serial.print(client.state());
			Serial.println(" try again... \n");
			delay(5000);
    }
  }
	Serial.print("connected \n");
  return client.connected();
}



void reconnect(void){
    long now = millis();
    if (now - last_reconnect_attempt > 5000) {
        last_reconnect_attempt = now;
        // Attempt to reconnect
        if (mqttReconnect()) {
        	last_reconnect_attempt = 0;
        }
    }
}

void loop(void) {
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop();
    tasker.loop();
  }
}


