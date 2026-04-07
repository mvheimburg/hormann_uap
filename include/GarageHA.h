#pragma once
#include <Arduino.h>

// Home Assistant MQTT Discovery (cover.mqtt) helper.
// - Discovery topic: <prefix>/cover/<unique_id>/config (retained)
// - availability_topic: cfg.topicStatus (online/offline)
// - command_topic:      cfg.topicCmd  (payloads: OPEN/CLOSE/STOP for cover,
//                      and ON/OFF for light)
// - state_topic:        <base>/cover/state  (retained)
// - position_topic:     <base>/cover/position (retained)
// - light switch state: <base>/light/state  (retained)
//
// Call haInit() after network is up, then haPublishDiscovery() after MQTT connect,
// and again whenever HA publishes birth message on "homeassistant/status".
//
// Use haSetState("open"/"opening"/"closed"/"closing"/"stopped") to update state.
// Use haSetPosition(0..100) for the cover position and haSetLightState("on"/"off")
// for the light entity.

void haInit(const char* discoveryPrefix = "homeassistant");
void haPublishDiscovery();
void haSetState(const char* state);
void haSetPosition(int position);
void haSetLightState(const char* state);
