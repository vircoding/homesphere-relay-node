#pragma once

#include <Bounce2.h>

#include <functional>
#include <map>

class SyncButtonManager {
 public:
  enum class Event {
    SIMPLE_PRESS,
    LONG_PRESS,
  };

  SyncButtonManager(const gpio_num_t buttonPin);
  void begin();
  void update();
  void on(Event event, std::function<void()> callback);

 private:
  static constexpr uint16_t LONG_PRESS_DURATION = 3000;
  static constexpr uint8_t DEBOUNCE_INTERVAL = 50;

  Bounce _debouncer;

  const gpio_num_t _PIN;
  unsigned long _pressStartTime;
  bool _longPressDetected;

  // Callbacks de eventos
  std::map<Event, std::function<void()>> _callbacks;

  // Metodos privados
  void _trigger(Event event);
};