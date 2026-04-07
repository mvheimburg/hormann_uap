#pragma once
#include <Arduino.h>
#include <PubSubClient.h>

// MQTT wrapper with auto-reconnect + LWT + cfg over MQTT.
// Call mqttBegin() once after Ethernet up.
// Call mqttEnsureConnected() frequently (non-blocking).
// Call mqttLoop() frequently.
// mqttSetHAOnlineHandler() is called after connect and when Home Assistant
// publishes its birth message to homeassistant/status.
//
// Optionally set a command handler (payload passed as C-string).
typedef void (*CmdHandler)(const char* payload);
typedef void (*SimpleHandler)();

void mqttBegin(CmdHandler handler = nullptr);
void mqttEnsureConnected();
void mqttLoop();
bool mqttPublishStatus(const char* payload, bool retain);
bool mqttPublishLarge(const char* topic, const char* payload, bool retain);
uint16_t mqttBufferSize();
PubSubClient* mqttClient();
void mqttSetHAOnlineHandler(SimpleHandler handler);
