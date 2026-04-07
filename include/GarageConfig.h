#pragma once
#include <Arduino.h>
#include <Ethernet.h>

void cfgInit();
uint8_t* cfgMac();
IPAddress cfgBrokerIP();
uint16_t cfgBrokerPort();
const char* cfgMqttClientId();
const char* cfgMqttUser();
const char* cfgMqttPass();
const char* cfgTopicCmd();
const char* cfgTopicStatus();
