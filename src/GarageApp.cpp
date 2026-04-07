#include "GarageApp.h"
#include "GarageConfig.h"
#include "GarageNet.h"
#include "GarageMqtt.h"
#include "GarageHA.h"

namespace {
constexpr uint8_t GPIO_STATE_GATE_OPEN = 2;
constexpr uint8_t GPIO_STATE_GATE_CLOSED = 3;
constexpr uint8_t GPIO_STATE_LIGHT_ON = 4;
constexpr uint8_t GPIO_CMD_GATE_OPEN = A0;
constexpr uint8_t GPIO_CMD_GATE_CLOSE = A1;
constexpr uint8_t GPIO_CMD_GATE_STOP = A2;
constexpr uint8_t GPIO_CMD_LIGHT = A3;
constexpr unsigned long SWITCH_DELAY_MS = 500;
constexpr unsigned long GATE_SPD_DOWN_SEC = 19;
constexpr unsigned long GATE_SPD_UP_SEC = 13;
constexpr int GATE_POS_OPEN = 100;
constexpr int GATE_POS_CLOSED = 0;

enum GatePinState { PIN_OPEN, PIN_CLOSED, PIN_BETWEEN };
enum GateState { GATE_OPEN, GATE_OPENING, GATE_CLOSED, GATE_CLOSING, GATE_STOPPED, GATE_UNKNOWN };
enum LightState { LIGHT_ON, LIGHT_OFF, LIGHT_UNKNOWN };

static GateState s_gateState = GATE_UNKNOWN;
static LightState s_lightState = LIGHT_UNKNOWN;
static int s_gatePosition = 0;
static unsigned long s_lastMoveAt = 0;

static bool sameToken(const char* lhs, const char* rhs) {
  if (!lhs || !rhs) return false;
  while (*lhs && *rhs) {
    char a = *lhs++;
    char b = *rhs++;
    if (a >= 'a' && a <= 'z') a -= 32;
    if (b >= 'a' && b <= 'z') b -= 32;
    if (a != b) return false;
  }
  return *lhs == 0 && *rhs == 0;
}

static const char* gateStateText(GateState state) {
  switch (state) {
    case GATE_OPEN: return "open";
    case GATE_OPENING: return "opening";
    case GATE_CLOSED: return "closed";
    case GATE_CLOSING: return "closing";
    case GATE_STOPPED: return "stopped";
    default: return "stopped";
  }
}

static const char* lightStateText(LightState state) {
  switch (state) {
    case LIGHT_ON: return "on";
    case LIGHT_OFF: return "off";
    default: return "off";
  }
}

static GatePinState readGatePins() {
  if (digitalRead(GPIO_STATE_GATE_OPEN) == LOW && digitalRead(GPIO_STATE_GATE_CLOSED) == HIGH) {
    return PIN_OPEN;
  }
  if (digitalRead(GPIO_STATE_GATE_OPEN) == HIGH && digitalRead(GPIO_STATE_GATE_CLOSED) == LOW) {
    return PIN_CLOSED;
  }
  return PIN_BETWEEN;
}

static LightState readLightPin() {
  return digitalRead(GPIO_STATE_LIGHT_ON) == HIGH ? LIGHT_ON : LIGHT_OFF;
}

static void triggerRelay(uint8_t pin) {
  digitalWrite(pin, LOW);
  delay(SWITCH_DELAY_MS);
  digitalWrite(pin, HIGH);
}

static void publishGateState() {
  haSetState(gateStateText(s_gateState));
}

static void publishGatePosition() {
  haSetPosition(s_gatePosition);
}

static void publishLightState() {
  haSetLightState(lightStateText(s_lightState));
}

static void updateLightState(bool forcePublish) {
  LightState prev = s_lightState;
  s_lightState = readLightPin();
  if (forcePublish || prev != s_lightState) {
    publishLightState();
  }
}

static void updateGateState(bool forcePublish) {
  GatePinState pinState = readGatePins();
  GateState prev = s_gateState;
  unsigned long now = millis();
  bool publishState = forcePublish;
  bool publishPosition = forcePublish;

  if (pinState == PIN_OPEN) {
    s_gateState = GATE_OPEN;
    s_gatePosition = GATE_POS_OPEN;
    s_lastMoveAt = now;
    publishState = forcePublish || prev != s_gateState;
    publishPosition = forcePublish || publishState;
  } else if (pinState == PIN_CLOSED) {
    s_gateState = GATE_CLOSED;
    s_gatePosition = GATE_POS_CLOSED;
    s_lastMoveAt = now;
    publishState = forcePublish || prev != s_gateState;
    publishPosition = forcePublish || publishState;
  } else {
    if (s_gateState == GATE_OPEN || s_gateState == GATE_OPENING) {
      s_gateState = GATE_OPENING;
    } else if (s_gateState == GATE_CLOSED || s_gateState == GATE_CLOSING) {
      s_gateState = GATE_CLOSING;
    } else if (s_gateState == GATE_UNKNOWN) {
      s_gateState = GATE_STOPPED;
    }

    if (s_gateState == GATE_OPENING || s_gateState == GATE_CLOSING) {
      unsigned long elapsed = now - s_lastMoveAt;
      if (elapsed >= 200 || forcePublish) {
        unsigned long speedSec = (s_gateState == GATE_OPENING) ? GATE_SPD_UP_SEC : GATE_SPD_DOWN_SEC;
        unsigned long delta = (elapsed * 100UL) / (speedSec * 1000UL);
        if (delta > 0 || forcePublish) {
          if (s_gateState == GATE_OPENING) {
            if ((int)delta + s_gatePosition >= GATE_POS_OPEN) {
              s_gatePosition = GATE_POS_OPEN;
            } else {
              s_gatePosition += (int)delta;
            }
          } else {
            if ((int)delta >= s_gatePosition) {
              s_gatePosition = GATE_POS_CLOSED;
            } else {
              s_gatePosition -= (int)delta;
            }
          }
          s_lastMoveAt = now;
          publishPosition = true;
        }
      }
    }

    publishState = forcePublish || prev != s_gateState;
  }

  if (publishState) {
    publishGateState();
  }
  if (publishPosition) {
    publishGatePosition();
  }
}

static void syncFromHardware(bool forcePublish) {
  updateGateState(forcePublish);
  updateLightState(forcePublish);
}

static void openGate() {
  triggerRelay(GPIO_CMD_GATE_OPEN);
  if (readGatePins() == PIN_BETWEEN) {
    s_gateState = GATE_OPENING;
    s_lastMoveAt = millis();
    publishGateState();
  } else {
    syncFromHardware(true);
  }
}

static void closeGate() {
  triggerRelay(GPIO_CMD_GATE_CLOSE);
  if (readGatePins() == PIN_BETWEEN) {
    s_gateState = GATE_CLOSING;
    s_lastMoveAt = millis();
    publishGateState();
  } else {
    syncFromHardware(true);
  }
}

static void stopGate() {
  triggerRelay(GPIO_CMD_GATE_STOP);
  if (readGatePins() == PIN_BETWEEN) {
    s_gateState = GATE_STOPPED;
    publishGateState();
  } else {
    syncFromHardware(true);
  }
}

static void triggerLight(bool on) {
  digitalWrite(GPIO_CMD_LIGHT, on ? HIGH : LOW);
  s_lightState = on ? LIGHT_ON : LIGHT_OFF;
  publishLightState();
}

static void handleCmd(const char* payload) {
  if (!payload || !*payload) return;

  if (sameToken(payload, "OPEN")) {
    openGate();
  } else if (sameToken(payload, "CLOSE")) {
    closeGate();
  } else if (sameToken(payload, "STOP")) {
    stopGate();
  } else if (sameToken(payload, "ON")) {
    triggerLight(true);
  } else if (sameToken(payload, "OFF")) {
    triggerLight(false);
  }
}

static void onMqttOnline() {
  haPublishDiscovery();
  syncFromHardware(true);
}

static void hardwareBegin() {
  pinMode(GPIO_STATE_GATE_OPEN, INPUT_PULLUP);
  pinMode(GPIO_STATE_GATE_CLOSED, INPUT_PULLUP);
  pinMode(GPIO_STATE_LIGHT_ON, INPUT_PULLUP);

  pinMode(GPIO_CMD_GATE_OPEN, OUTPUT);
  digitalWrite(GPIO_CMD_GATE_OPEN, HIGH);
  pinMode(GPIO_CMD_GATE_CLOSE, OUTPUT);
  digitalWrite(GPIO_CMD_GATE_CLOSE, HIGH);
  pinMode(GPIO_CMD_GATE_STOP, OUTPUT);
  digitalWrite(GPIO_CMD_GATE_STOP, HIGH);
  pinMode(GPIO_CMD_LIGHT, OUTPUT);
  digitalWrite(GPIO_CMD_LIGHT, HIGH);

  syncFromHardware(true);
}
}  // namespace

void appSetup() {
  Serial.begin(115200);
  while (!Serial) { ; }

  if (!cfgLoad(g_cfg)) {
    cfgSetDefaults(g_cfg);
    cfgSave(g_cfg);
    Serial.println(F("Default config saved to EEPROM."));
  } else {
    Serial.println(F("Config loaded from EEPROM."));
  }
  GarageConfig loadedCfg = g_cfg;
  cfgApplySecrets(g_cfg);
  if (memcmp(&loadedCfg, &g_cfg, sizeof(GarageConfig) - sizeof(uint16_t)) != 0) {
    cfgSave(g_cfg);
    Serial.println(F("Applied overrides from secrets.h."));
  }

  hardwareBegin();

  netBegin();
  mqttBegin(handleCmd);
  haInit("homeassistant");
  mqttSetHAOnlineHandler(onMqttOnline);

  Serial.print(F("IP: ")); Serial.println(netLocalIP());
  Serial.print(F("Broker: ")); Serial.print(g_cfg.broker); Serial.print(F(":")); Serial.println(g_cfg.port);
  Serial.print(F("MQTT cmd: ")); Serial.println(g_cfg.topicCmd);
  Serial.print(F("MQTT status: ")); Serial.println(g_cfg.topicStatus);
}

void appLoop() {
  netMaintain();
  mqttEnsureConnected();
  mqttLoop();
  syncFromHardware(false);
}
