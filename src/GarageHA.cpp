#include "GarageHA.h"
#include <ArduinoHA.h>
#if __has_include("secrets.h")
#include "secrets.h"
#endif

namespace {
#ifdef GATE_POSITION_ENABLED
constexpr bool GATE_POS_ENABLED = (GATE_POSITION_ENABLED != 0);
#else
constexpr bool GATE_POS_ENABLED = false;
#endif
HaCmdHandler s_cmdHandler = nullptr;
HACover* s_cover = nullptr;
HASwitch* s_light = nullptr;
bool s_ready = false;

void onCoverCommand(HACover::CoverCommand cmd, HACover*) {
  if (!s_cmdHandler) return;
  if (cmd == HACover::CommandOpen) {
    s_cmdHandler("OPEN");
  } else if (cmd == HACover::CommandClose) {
    s_cmdHandler("CLOSE");
  } else if (cmd == HACover::CommandStop) {
    s_cmdHandler("STOP");
  }
}

void onLightCommand(bool state, HASwitch*) {
  if (!s_cmdHandler) return;
  s_cmdHandler(state ? "ON" : "OFF");
}
}  // namespace

void haInit(const char* discoveryPrefix) {
  (void)discoveryPrefix;
  if (s_ready) return;

  // HAMqtt singleton must be initialized in mqttBegin() before this.
  s_cover = new HACover(
      "gate",
      GATE_POS_ENABLED ? HACover::PositionFeature : HACover::DefaultFeatures);
  s_light = new HASwitch("light");
  if (!s_cover || !s_light) {
    Serial.println(F("[HA] Entity allocation failed"));
    return;
  }

  s_cover->setName("Garage Gate");
  s_cover->setDeviceClass("garage");
  s_cover->onCommand(onCoverCommand);

  s_light->setName("Garage Light");
  s_light->onCommand(onLightCommand);
  s_ready = true;
}

void haSetCommandHandler(HaCmdHandler handler) {
  s_cmdHandler = handler;
}

void haPublishDiscovery() {
  // ArduinoHA publishes discovery automatically when MQTT connects.
}

void haSetState(const char* state) {
  if (!s_ready || !s_cover || !state) return;

  if (strcmp(state, "open") == 0) {
    s_cover->setState(HACover::StateOpen);
  } else if (strcmp(state, "opening") == 0) {
    s_cover->setState(HACover::StateOpening);
  } else if (strcmp(state, "closed") == 0) {
    s_cover->setState(HACover::StateClosed);
  } else if (strcmp(state, "closing") == 0) {
    s_cover->setState(HACover::StateClosing);
  } else {
    s_cover->setState(HACover::StateStopped);
  }
}

void haSetPosition(int position) {
  if (!GATE_POS_ENABLED || !s_ready || !s_cover) return;
  if (position < 0) position = 0;
  if (position > 100) position = 100;
  s_cover->setPosition(position);
}

void haSetLightState(const char* state) {
  if (!s_ready || !s_light || !state) return;
  s_light->setState(strcmp(state, "on") == 0);
}
