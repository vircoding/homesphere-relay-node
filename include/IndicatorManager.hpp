#pragma once

#include <Arduino.h>

class IndicatorManager {
 public:
  IndicatorManager(const gpio_num_t ledPin);
  void begin();
  void set(const bool status);

 private:
  const gpio_num_t _LED_PIN;
};