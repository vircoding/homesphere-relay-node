#include "IndicatorManager.hpp"

IndicatorManager::IndicatorManager(const gpio_num_t ledPin)
    : _LED_PIN(ledPin) {}

void IndicatorManager::begin() {
  pinMode(_LED_PIN, OUTPUT);
  set(false);
}

void IndicatorManager::set(const bool status) {
  if (status)
    digitalWrite(_LED_PIN, HIGH);
  else
    digitalWrite(_LED_PIN, LOW);
}
