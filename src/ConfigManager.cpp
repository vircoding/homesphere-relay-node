#include "ConfigManager.hpp"

#include <LittleFS.h>

#include "Utils.hpp"

bool ConfigManager::init() {
  if (!LittleFS.begin()) return false;
  return _loadConfig();
}

bool ConfigManager::saveMasterMacConfig(const uint8_t* masterMac) {
  File configFile = LittleFS.open("/config.json", "r");
  JsonDocument doc;

  if (deserializeJson(doc, configFile)) {
    configFile.close();
    return false;
  }

  configFile.close();
  doc["master_mac"] = macToString(masterMac);

  return _writeConfig(doc);
}

void ConfigManager::copyMasterMac(uint8_t* macDest) {
  memcpy(macDest, _masterMac, 6);
}

bool ConfigManager::_loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) return false;

  JsonDocument doc;
  if (deserializeJson(doc, configFile)) {
    configFile.close();
    return false;
  }

  if (doc["master_mac"].is<const char*>()) {
    const char* macStr = doc["master_mac"];
    stringToMac(macStr, _masterMac);
  } else {
    stringToMac("FFXFFXFFXFFXFFXFF", _masterMac);
  }

  configFile.close();
  return true;
}

bool ConfigManager::_writeConfig(const JsonDocument& doc) {
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) return false;

  serializeJsonPretty(doc, configFile);
  configFile.close();
  return true;
}
