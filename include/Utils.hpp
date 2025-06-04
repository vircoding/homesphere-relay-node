#pragma once

#include <Arduino.h>
#include <CRC8.h>

String macToString(const uint8_t* mac);
void stringToMac(const String& macStr, uint8_t* macDest);
uint8_t calcCRC8(const uint8_t* data, size_t length);

template <typename T>
void addCRC8(T& msg) {
  uint8_t* crcField = (uint8_t*)&msg + sizeof(T) - 1;
  *crcField = calcCRC8((uint8_t*)&msg, sizeof(T) - 1);
}

template <typename T>
bool verifyCRC8(const T& msg) {
  CRC8 crc(CRC8_DALLAS_MAXIM_POLYNOME);
  crc.reset();

  const uint8_t* data = reinterpret_cast<const uint8_t*>(&msg);

  crc.add(data, sizeof(T) - 1);

  return crc.calc() == data[sizeof(T) - 1];
}
