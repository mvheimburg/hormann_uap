#pragma once
#include <Arduino.h>

typedef void (*HaCmdHandler)(const char* payload);

void haInit(const char* discoveryPrefix = "homeassistant");
void haSetCommandHandler(HaCmdHandler handler);
void haPublishDiscovery();
void haSetState(const char* state);
void haSetPosition(int position);
void haSetLightState(const char* state);
