#include "GarageHA.h"
#include "GarageConfig.h"
#include "GarageNet.h"
#include "GarageMqtt.h"
#include <Ethernet.h>
#include <PubSubClient.h>
#include <stdio.h>

static void macToStr(const uint8_t mac[6], char* out, size_t n) {
  snprintf(out, n, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void buildUniqueId(char* out, size_t n) {
  snprintf(out, n, "garage_%02X%02X%02X%02X%02X%02X",
           g_cfg.mac[0], g_cfg.mac[1], g_cfg.mac[2], g_cfg.mac[3], g_cfg.mac[4], g_cfg.mac[5]);
}

static void buildBaseTopic(char* out, size_t n) {
  strncpy(out, g_cfg.topicStatus, n - 1);
  out[n - 1] = 0;
  char* slash = strrchr(out, '/');
  if (slash && strcmp(slash, "/status") == 0) {
    *slash = 0;
  }
}

static void buildTopic(char* out, size_t n, const char* suffix) {
  char base[48];
  buildBaseTopic(base, sizeof(base));
  snprintf(out, n, "%s/%s", base, suffix);
}

void haInit(const char* discoveryPrefix) {
  (void)discoveryPrefix;
}

void haPublishDiscovery() {
  char uniqueId[48];
  char macFlat[13];
  char macStr[24];
  char name[40];
  char lightName[40];
  char discTopic[96];
  char lightDiscTopic[96];
  char stateTopic[64];
  char positionTopic[64];
  char lightStateTopic[64];
  char json[768];
  char lightJson[512];
  IPAddress ip = netLocalIP();

  snprintf(macFlat, sizeof(macFlat), "%02X%02X%02X%02X%02X%02X",
           g_cfg.mac[0], g_cfg.mac[1], g_cfg.mac[2], g_cfg.mac[3], g_cfg.mac[4], g_cfg.mac[5]);
  buildUniqueId(uniqueId, sizeof(uniqueId));
  macToStr(g_cfg.mac, macStr, sizeof(macStr));
  snprintf(name, sizeof(name), "Garage Door %s", macFlat + 6);
  snprintf(lightName, sizeof(lightName), "Garage Light %s", macFlat + 6);
  snprintf(discTopic, sizeof(discTopic), "homeassistant/cover/%s/config", uniqueId);
  snprintf(lightDiscTopic, sizeof(lightDiscTopic), "homeassistant/switch/%s_light/config", uniqueId);
  buildTopic(stateTopic, sizeof(stateTopic), "cover/state");
  buildTopic(positionTopic, sizeof(positionTopic), "cover/position");
  buildTopic(lightStateTopic, sizeof(lightStateTopic), "light/state");

  int n = snprintf(json, sizeof(json),
    "{"
      "\"name\":\"%s\","
      "\"unique_id\":\"%s\","
      "\"device_class\":\"garage\","
      "\"command_topic\":\"%s\","
      "\"state_topic\":\"%s\","
      "\"position_topic\":\"%s\","
      "\"availability_topic\":\"%s\","
      "\"payload_available\":\"online\","
      "\"payload_not_available\":\"offline\","
      "\"payload_open\":\"OPEN\","
      "\"payload_close\":\"CLOSE\","
      "\"payload_stop\":\"STOP\","
      "\"state_open\":\"open\","
      "\"state_opening\":\"opening\","
      "\"state_closed\":\"closed\","
      "\"state_closing\":\"closing\","
      "\"state_stopped\":\"stopped\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"connections\":[[\"mac\",\"%s\"]],"
        "\"manufacturer\":\"Keyestudio\",\"model\":\"W5500 Garage Controller\""
      "},"
      "\"configuration_url\":\"http://%u.%u.%u.%u/\""
    "}",
    name, uniqueId, g_cfg.topicCmd, stateTopic, positionTopic, g_cfg.topicStatus,
    uniqueId, macStr, ip[0], ip[1], ip[2], ip[3]
  );
  if (n < 0 || n >= (int)sizeof(json)) return;

  int m = snprintf(lightJson, sizeof(lightJson),
    "{"
      "\"name\":\"%s\","
      "\"unique_id\":\"%s_light\","
      "\"command_topic\":\"%s\","
      "\"state_topic\":\"%s\","
      "\"availability_topic\":\"%s\","
      "\"payload_available\":\"online\","
      "\"payload_not_available\":\"offline\","
      "\"payload_on\":\"ON\","
      "\"payload_off\":\"OFF\","
      "\"state_on\":\"on\","
      "\"state_off\":\"off\","
      "\"icon\":\"mdi:lightbulb\","
      "\"device\":{"
        "\"identifiers\":[\"%s\"],"
        "\"connections\":[[\"mac\",\"%s\"]],"
        "\"manufacturer\":\"Keyestudio\",\"model\":\"W5500 Garage Controller\""
      "}"
    "}",
    lightName, uniqueId, g_cfg.topicCmd, lightStateTopic, g_cfg.topicStatus,
    uniqueId, macStr
  );
  if (m < 0 || m >= (int)sizeof(lightJson)) return;

  auto* client = mqttClient();
  if (client && client->connected()) {
    client->publish(discTopic, json, true);
    client->publish(lightDiscTopic, lightJson, true);
  }
}

void haSetState(const char* state) {
  auto* client = mqttClient();
  if (!client || !client->connected()) return;
  char topic[64];
  buildTopic(topic, sizeof(topic), "cover/state");
  client->publish(topic, state ? state : "", true);
}

void haSetPosition(int position) {
  auto* client = mqttClient();
  if (!client || !client->connected()) return;
  if (position < 0) position = 0;
  if (position > 100) position = 100;
  char topic[64];
  char pos[8];
  buildTopic(topic, sizeof(topic), "cover/position");
  itoa(position, pos, 10);
  client->publish(topic, pos, true);
}

void haSetLightState(const char* state) {
  auto* client = mqttClient();
  if (!client || !client->connected()) return;
  char topic[64];
  buildTopic(topic, sizeof(topic), "light/state");
  client->publish(topic, state ? state : "", true);
}
