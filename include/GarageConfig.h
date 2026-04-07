#pragma once
#include <Arduino.h>

// Persistent config stored in EEPROM with CRC
struct GarageConfig {
  uint32_t magic;            // 0xC0FFEE01
  char broker[16];           // hostname or IP
  uint16_t port;             // MQTT port
  char user[8];
  char pass[8];
  char clientId[16];
  char topicCmd[22];
  char topicStatus[22];
  uint8_t mac[6];            // NIC MAC address
  uint16_t crc;
};

extern GarageConfig g_cfg;

bool cfgLoad(GarageConfig& out);
void cfgSetDefaults(GarageConfig& cfg);
void cfgApplySecrets(GarageConfig& cfg);
void cfgSave(GarageConfig& cfg);
uint16_t cfgCrc(const GarageConfig& c);

const uint32_t CFG_MAGIC = 0xC0FFEE01;
const int EEPROM_ADDR = 0;   // EEPROM start
