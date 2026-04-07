#include "GarageConfig.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "include/secrets.h is required"
#endif

#ifndef MQTT_TOPIC_CMD
#define MQTT_TOPIC_CMD "garage/gate/cmd"
#endif

#ifndef MQTT_TOPIC_STATUS
#define MQTT_TOPIC_STATUS "garage/gate/status"
#endif

namespace {
static const uint8_t kMac[6] = MAC;
static const uint8_t kBrokerIp[4] = BROKER;
}

void cfgInit() {
}

uint8_t* cfgMac() {
  return const_cast<uint8_t*>(kMac);
}

IPAddress cfgBrokerIP() {
  return IPAddress(kBrokerIp[0], kBrokerIp[1], kBrokerIp[2], kBrokerIp[3]);
}

uint16_t cfgBrokerPort() {
  return BROKER_PORT;
}

const char* cfgMqttClientId() {
  return MQTT_CLIENT_ID;
}

const char* cfgMqttUser() {
  return MQTT_USER;
}

const char* cfgMqttPass() {
  return MQTT_PWD;
}

const char* cfgTopicCmd() {
  return MQTT_TOPIC_CMD;
}

const char* cfgTopicStatus() {
  return MQTT_TOPIC_STATUS;
}
