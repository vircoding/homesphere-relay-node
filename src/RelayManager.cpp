#include "RelayManager.hpp"

RelayManager::RelayManager(const gpio_num_t relayPin) : _RELAY_PIN(relayPin) {}

RelayManager::~RelayManager() {
  if (_offsetTimerHandler) xTimerDelete(_offsetTimerHandler, 0);
  if (_durationTimerHandler) xTimerDelete(_durationTimerHandler, 0);
}

void RelayManager::begin() {
  pinMode(_RELAY_PIN, OUTPUT);
  set(false);
}

void RelayManager::set(const bool state) {
  digitalWrite(_RELAY_PIN, state);
  if (_stateCallback) _stateCallback();
}

void RelayManager::schedule(const uint32_t offset, const uint32_t duration) {
  // Cancelar timers existentes
  if (_offsetTimerHandler) {
    Serial.println("Borrando timer offset");
    xTimerDelete(_offsetTimerHandler, 0);
  }

  if (_durationTimerHandler) {
    Serial.println("Borrando timer duration");
    xTimerDelete(_durationTimerHandler, 0);
  }

  // Verificar duraciones mayores que 0
  if (duration == 0) return;
  Serial.println("Duration mayor que 0");

  // Crear el timer para offset (activación)
  if (offset > 0) {
    Serial.println("Timer offset creado");
    _offsetTimerHandler = xTimerCreate("Offset", pdMS_TO_TICKS(offset), pdFALSE,
                                       this, _timerCallback);

    // Iniciar timer
    if (_offsetTimerHandler) xTimerStart(_offsetTimerHandler, 0);
  } else {
    Serial.println("Activando relay inmediatamente");
    // Activar el relay inmediatamente
    set(true);
  }

  // Crear timer para duration (desactivación)
  if (duration != 0xFFFFFFFF) {
    Serial.println("Timer duration creado");
    _durationTimerHandler =
        xTimerCreate("Duration", pdMS_TO_TICKS(offset + duration), pdFALSE,
                     this, _timerCallback);

    if (_durationTimerHandler) xTimerStart(_durationTimerHandler, 0);
  }
}

void RelayManager::_timerCallback(TimerHandle_t xTimer) {
  RelayManager* instance =
      static_cast<RelayManager*>(pvTimerGetTimerID(xTimer));

  if (xTimer == instance->_offsetTimerHandler) {
    instance->set(true);  // Activar relay

    xTimerStop(instance->_offsetTimerHandler, 0);  // Detener el timer
  } else if (xTimer == instance->_durationTimerHandler) {
    instance->set(false);  // Desactivar el relay

    xTimerStop(instance->_durationTimerHandler, 0);  // Detener el timer
  }
}

void RelayManager::cancelSchedule() {
  if (_offsetTimerHandler) {
    xTimerStop(_offsetTimerHandler, 0);
    xTimerDelete(_offsetTimerHandler, 0);
    _offsetTimerHandler = nullptr;
  }

  if (_durationTimerHandler) {
    xTimerStop(_durationTimerHandler, 0);
    xTimerDelete(_durationTimerHandler, 0);
    _durationTimerHandler = nullptr;
  }
}
