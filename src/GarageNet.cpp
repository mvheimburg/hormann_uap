#include "GarageNet.h"
#include "GarageConfig.h"

#if __has_include("secrets.h")
#include "secrets.h"
#define GARAGE_HAS_SECRETS 1
#else
#define GARAGE_HAS_SECRETS 0
#endif

void netBegin() {
  Ethernet.init(ETH_CS);
  // Static IPv4 avoids DHCP RAM overhead on the Uno.
  IPAddress ip(192,168,1,200);
  IPAddress dns(192,168,1,1);
  IPAddress gw(192,168,1,1);
  IPAddress mask(255,255,255,0);
#if GARAGE_HAS_SECRETS && defined(DEVICE_IP)
  uint8_t cfgIp[4] = DEVICE_IP;
  ip = IPAddress(cfgIp[0], cfgIp[1], cfgIp[2], cfgIp[3]);
#endif
#if GARAGE_HAS_SECRETS && defined(DEVICE_DNS)
  uint8_t cfgDns[4] = DEVICE_DNS;
  dns = IPAddress(cfgDns[0], cfgDns[1], cfgDns[2], cfgDns[3]);
#endif
#if GARAGE_HAS_SECRETS && defined(DEVICE_GW)
  uint8_t cfgGw[4] = DEVICE_GW;
  gw = IPAddress(cfgGw[0], cfgGw[1], cfgGw[2], cfgGw[3]);
#endif
#if GARAGE_HAS_SECRETS && defined(DEVICE_MASK)
  uint8_t cfgMask[4] = DEVICE_MASK;
  mask = IPAddress(cfgMask[0], cfgMask[1], cfgMask[2], cfgMask[3]);
#endif
  Ethernet.begin(cfgMac(), ip, dns, gw, mask);
  delay(100);
}

void netMaintain() {
}

IPAddress netLocalIP() {
  return Ethernet.localIP();
}
