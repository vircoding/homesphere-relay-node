#include "Utils.hpp"

String macToString(const uint8_t* mac) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(buffer);
}

void stringToMac(const String& macStr, uint8_t* macDest) {
  if (macStr.length() != 17) return;

  sscanf(macStr.c_str(), "%2hhX:%2hhX:%2hhX:%2hhX:%2hhX:%2hhX", &macDest[0],
         &macDest[1], &macDest[2], &macDest[3], &macDest[4], &macDest[5]);
}

uint8_t calcCRC8(const uint8_t* data, size_t length) {
  CRC8 crc(CRC8_DALLAS_MAXIM_POLYNOME);
  crc.reset();
  crc.add(data, length);

  return crc.calc();
}