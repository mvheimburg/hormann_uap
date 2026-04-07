#include "GarageConfig.h"
#include <EEPROM.h>

#if __has_include("secrets.h")
#include "secrets.h"
#define GARAGE_HAS_SECRETS 1
#else
#define GARAGE_HAS_SECRETS 0
#endif

static uint16_t crc16_update(uint16_t crc, uint8_t a) {
  crc ^= a;
  for (int i = 0; i < 8; ++i) {
    if (crc & 1) crc = (crc >> 1) ^ 0xA001;
    else crc = (crc >> 1);
  }
  return crc;
}

static void copyStr(char* dst, size_t dstSize, const char* src) {
  if (!dst || dstSize == 0) return;
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

uint16_t cfgCrc(const GarageConfig& c) {
  const uint8_t* p = (const uint8_t*)&c;
  size_t n = sizeof(GarageConfig) - sizeof(uint16_t);
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < n; ++i) crc = crc16_update(crc, p[i]);
  return crc;
}

GarageConfig g_cfg;

static void eepromWriteConfig(const GarageConfig& c) {
  const uint8_t* p = (const uint8_t*)&c;
  for (unsigned int i = 0; i < sizeof(GarageConfig); ++i) {
    EEPROM.update(EEPROM_ADDR + i, p[i]);
  }
}

static void eepromReadConfig(GarageConfig& out) {
  uint8_t* p = (uint8_t*)&out;
  for (unsigned int i = 0; i < sizeof(GarageConfig); ++i) {
    p[i] = EEPROM.read(EEPROM_ADDR + i);
  }
}

bool cfgLoad(GarageConfig& out) {
  eepromReadConfig(out);
  if (out.magic != CFG_MAGIC) return false;
  uint16_t calc = cfgCrc(out);
  return (calc == out.crc);
}

void cfgSetDefaults(GarageConfig& cfg) {
  memset(&cfg, 0, sizeof(cfg));
  cfg.magic = CFG_MAGIC;
  copyStr(cfg.broker, sizeof(cfg.broker), "192.168.1.5");
  cfg.port = 1883;
  copyStr(cfg.user, sizeof(cfg.user), "");
  copyStr(cfg.pass, sizeof(cfg.pass), "");
  copyStr(cfg.clientId, sizeof(cfg.clientId), "garage-w5500");
  copyStr(cfg.topicCmd, sizeof(cfg.topicCmd), "garage/door-1/cmd");
  copyStr(cfg.topicStatus, sizeof(cfg.topicStatus), "garage/door-1/status");
  cfg.mac[0]=0x02; cfg.mac[1]=0x11; cfg.mac[2]=0x22; cfg.mac[3]=0x33; cfg.mac[4]=0x44; cfg.mac[5]=0x55;
  cfg.crc = cfgCrc(cfg);
}

void cfgApplySecrets(GarageConfig& cfg) {
#if GARAGE_HAS_SECRETS
  uint8_t brokerIp[4] = BROKER;
  char broker[sizeof(cfg.broker)];
  snprintf(
      broker, sizeof(broker), "%u.%u.%u.%u",
      brokerIp[0], brokerIp[1], brokerIp[2], brokerIp[3]);
  copyStr(cfg.broker, sizeof(cfg.broker), broker);
  cfg.port = BROKER_PORT;
  copyStr(cfg.clientId, sizeof(cfg.clientId), MQTT_CLIENT_ID);
  copyStr(cfg.user, sizeof(cfg.user), MQTT_USER);
  copyStr(cfg.pass, sizeof(cfg.pass), MQTT_PWD);
  uint8_t mac[6] = MAC;
  memcpy(cfg.mac, mac, sizeof(cfg.mac));
#endif
}

void cfgSave(GarageConfig& cfg) {
  cfg.crc = cfgCrc(cfg);
  eepromWriteConfig(cfg);
}
