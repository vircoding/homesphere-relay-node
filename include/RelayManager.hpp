#pragma once

#include <Arduino.h>

#include <functional>

class RelayManager {
 public:
  RelayManager(const gpio_num_t relayPin);
  ~RelayManager();
  void begin();
  void set(const bool state);
  void schedule(const uint32_t offset = 0,
                const uint32_t duration = 0xFFFFFFFF);
  void cancelSchedule();
  bool getState() const { return digitalRead(_RELAY_PIN); }
  void onState(std::function<void()> callback) { _stateCallback = callback; }

 private:
  const gpio_num_t _RELAY_PIN;

  // Timer Handlers
  TimerHandle_t _offsetTimerHandler = nullptr;
  TimerHandle_t _durationTimerHandler = nullptr;

  std::function<void()> _stateCallback;

  static void _timerCallback(TimerHandle_t xTimer);
};