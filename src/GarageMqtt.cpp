#include "GarageMqtt.h"
#include "GarageConfig.h"
#include <Ethernet.h>
#include <PubSubClient.h>
#include <avr/pgmspace.h>

static EthernetClient s_eth;
static PubSubClient s_mqtt(s_eth);

static const unsigned long KEEPALIVE = 15;
static const unsigned long SOCKET_TIMEOUT = 15;
static unsigned long s_lastAttempt = 0;
static unsigned long s_backoff = 2000;
static const unsigned long BACKOFF_MAX = 60000;
static CmdHandler s_handler = nullptr;
static SimpleHandler s_haOnlineHandler = nullptr;

static const char LWT_ON[] = "online";
static const char LWT_OFF[] = "offline";

static void subscribeAll() {
  char haTopic[] = "homeassistant/status";
  s_mqtt.subscribe(g_cfg.topicCmd);
  s_mqtt.subscribe(haTopic);
}

static bool isOnlinePayload(const char* payload) {
  return payload && (strcmp(payload, "online") == 0 || strcmp(payload, "birth") == 0);
}

static void notifyHaOnline() {
  if (s_haOnlineHandler) s_haOnlineHandler();
}

static void mqttMsg(char* topic, byte* payload, unsigned int len) {
  // ensure null-terminated small buffer
  static char buf[16];
  unsigned int n = (len < sizeof(buf)-1) ? len : sizeof(buf)-1;
  memcpy(buf, payload, n);
  buf[n] = 0;

  if (strcmp(topic, g_cfg.topicCmd) == 0) {
    if (s_handler) s_handler(buf);
    return;
  }

  if (strcmp_P(topic, PSTR("homeassistant/status")) == 0 && isOnlinePayload(buf)) {
    notifyHaOnline();
  }
}

void mqttBegin(CmdHandler handler) {
  s_handler = handler;
  s_mqtt.setServer(g_cfg.broker, g_cfg.port);
  s_mqtt.setKeepAlive(KEEPALIVE);
  s_mqtt.setSocketTimeout(SOCKET_TIMEOUT);
  s_mqtt.setBufferSize(256);
  s_mqtt.setCallback(mqttMsg);
}

static bool connectNow() {
  if (strlen(g_cfg.user) > 0) {
    if (s_mqtt.connect(g_cfg.clientId, g_cfg.user, g_cfg.pass, g_cfg.topicStatus, 1, true, LWT_OFF, true)) {
      s_mqtt.publish(g_cfg.topicStatus, LWT_ON, true);
      subscribeAll();
      s_backoff = 2000;
      notifyHaOnline();
      return true;
    }
  } else {
    if (s_mqtt.connect(g_cfg.clientId, nullptr, nullptr, g_cfg.topicStatus, 1, true, LWT_OFF, true)) {
      s_mqtt.publish(g_cfg.topicStatus, LWT_ON, true);
      subscribeAll();
      s_backoff = 2000;
      notifyHaOnline();
      return true;
    }
  }
  return false;
}

void mqttEnsureConnected() {
  if (s_mqtt.connected()) return;
  unsigned long now = millis();
  if (now - s_lastAttempt >= s_backoff) {
    s_lastAttempt = now;
    s_mqtt.setServer(g_cfg.broker, g_cfg.port); // in case cfg changed
    if (!connectNow()) {
      if (s_backoff < BACKOFF_MAX) s_backoff *= 2;
      if (s_backoff > BACKOFF_MAX) s_backoff = BACKOFF_MAX;
    }
  }
}

void mqttLoop() {
  s_mqtt.loop();
}

bool mqttPublishStatus(const char* payload, bool retain) {
  if (!s_mqtt.connected()) return false;
  return s_mqtt.publish(g_cfg.topicStatus, payload, retain);
}

PubSubClient* mqttClient() { return &s_mqtt; }

void mqttSetHAOnlineHandler(SimpleHandler handler) {
  s_haOnlineHandler = handler;
}
