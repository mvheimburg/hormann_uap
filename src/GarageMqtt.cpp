#include "GarageMqtt.h"
#include "GarageConfig.h"
#include "GarageHA.h"
#include <ArduinoHA.h>
#include <Ethernet.h>

namespace {
static EthernetClient s_eth;
static HADevice s_device;
static HAMqtt* s_mqtt = nullptr;
static SimpleHandler s_haOnlineHandler = nullptr;

void onConnected() {
  Serial.println(F("[MQTT] CONNECT OK (ArduinoHA)"));
  if (s_haOnlineHandler) s_haOnlineHandler();
}

void onStateChanged(HAMqtt::ConnectionState state) {
  if (state == HAMqtt::StateConnected) return;
  Serial.print(F("[MQTT] State="));
  Serial.println((int)state);
}
}  // namespace

void mqttBegin(CmdHandler handler) {
  haSetCommandHandler(handler);

  if (!s_device.getUniqueId()) {
    s_device.setUniqueId(cfgMac(), 6);
    s_device.enableExtendedUniqueIds();
    s_device.setName("Garage Controller");
    s_device.setManufacturer("DIY");
    s_device.setModel("W5500 Gate");
  }

  if (!s_mqtt) {
    s_mqtt = new HAMqtt(s_eth, s_device, 6);
    if (!s_mqtt) {
      Serial.println(F("[MQTT] HAMqtt allocation failed"));
      return;
    }
    s_mqtt->setDiscoveryPrefix("homeassistant");
    s_mqtt->setKeepAlive(15);
    s_mqtt->setBufferSize(256);
    s_mqtt->onConnected(onConnected);
    s_mqtt->onStateChanged(onStateChanged);
  }

  if (strlen(cfgMqttUser()) > 0) {
    s_mqtt->begin(cfgBrokerIP(), cfgBrokerPort(), cfgMqttUser(), cfgMqttPass());
  } else {
    s_mqtt->begin(cfgBrokerIP(), cfgBrokerPort());
  }
}

void mqttEnsureConnected() {
  // ArduinoHA manages reconnects internally from mqttLoop().
}

void mqttLoop() {
  if (s_mqtt) s_mqtt->loop();
}

bool mqttPublishStatus(const char* payload, bool retain) {
  if (!s_mqtt) return false;
  return s_mqtt->publish(cfgTopicStatus(), payload ? payload : "", retain);
}

bool mqttPublishLarge(const char* topic, const char* payload, bool retain) {
  if (!s_mqtt || !topic || !payload) return false;
  const uint16_t n = (uint16_t)strlen(payload);
  if (!s_mqtt->beginPublish(topic, n, retain)) return false;
  s_mqtt->writePayload(payload, n);
  return s_mqtt->endPublish();
}

uint16_t mqttBufferSize() {
  return 256;
}

PubSubClient* mqttClient() {
  return nullptr;
}

void mqttSetHAOnlineHandler(SimpleHandler handler) {
  s_haOnlineHandler = handler;
}
