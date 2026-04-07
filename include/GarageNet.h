#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>

#ifndef ETH_CS
#define ETH_CS 10   // W5500 CS on most UNO-based boards
#endif

void netBegin();
void netMaintain();
IPAddress netLocalIP();
