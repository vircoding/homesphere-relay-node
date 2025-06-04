#pragma once

#include <ArduinoJson.h>

class ConfigManager {
 public:
  bool init();
  bool saveMasterMacConfig(const uint8_t* masterMac);
  void copyMasterMac(uint8_t* macDest);

 private:
  uint8_t _masterMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  bool _loadConfig();
  bool _writeConfig(const JsonDocument& doc);
};