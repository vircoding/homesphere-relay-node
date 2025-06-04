#include <LittleFS.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>

#include "ConfigManager.hpp"
#include "IndicatorManager.hpp"
#include "NowManager.hpp"
#include "RelayManager.hpp"
#include "SyncButtonManager.hpp"
#include "Utils.hpp"

// Pinout
const gpio_num_t relayPin = GPIO_NUM_32;
const gpio_num_t ledPin = GPIO_NUM_2;
const gpio_num_t syncButtonPin = GPIO_NUM_23;

// Task Handlers
TaskHandle_t blinkLEDTaskHandler = NULL;
TaskHandle_t sendActuatorStateTaskHandler = NULL;

// Timer Handlers
TimerHandle_t syncModeTimeoutTimerHandler = NULL;

// Global Variables
bool syncModeState = false;

RelayManager relay(relayPin);
ConfigManager config;
IndicatorManager led(ledPin);
SyncButtonManager syncButton(syncButtonPin);
NowManager now;

// Definitions
void onReceivedCallback(const uint8_t* mac, const uint8_t* data, int length);
void onSendCallback(const uint8_t* mac, esp_now_send_status_t status);
void onSyncReceivedCallback(const uint8_t* mac, const uint8_t* data,
                            int length);
void onConfirmRegistrationReceivedCallback(const uint8_t* mac,
                                           const uint8_t* data, int length);
void blinkLEDTask(void* parameter);
void enterSyncMode();
void endSyncMode();
void onLongButtonPressCallback() { enterSyncMode(); };
void onSimpleButtonPressCallback();
void syncModeTimeoutCallback(TimerHandle_t xTimer) { endSyncMode(); };
void registerMasterNode();
void sendActuatorStateTask(void* parameter);
void enableDataTransfer();
void disableDataTransfer();
void onSetCallback();

void setup() {
  Serial.begin(115200);

  if (!config.init()) {
    ESP.restart();
  }

  WiFi.mode(WIFI_MODE_STA);

  relay.begin();
  relay.onState(onSetCallback);

  led.begin();

  syncButton.begin();
  syncButton.on(SyncButtonManager::Event::SIMPLE_PRESS,
                onSimpleButtonPressCallback);
  syncButton.on(SyncButtonManager::Event::LONG_PRESS,
                onLongButtonPressCallback);

  now.init();
  now.onReceived(onReceivedCallback);
  now.onSend(onSendCallback);
  enableDataTransfer();
  registerMasterNode();
}

void loop() { syncButton.update(); }

void onReceivedCallback(const uint8_t* mac, const uint8_t* data, int length) {
  if (!now.isMasterMac(mac)) return;

  if (NowManager::validateMessage(NowManager::MessageType::SET_ACTUATOR, data,
                                  length)) {
    const NowManager::SetActuatorMsg* msg =
        reinterpret_cast<const NowManager::SetActuatorMsg*>(data);

    if (verifyCRC8(*msg)) {
      relay.cancelSchedule();
      relay.set(msg->state);
    }
  } else if (NowManager::validateMessage(
                 NowManager::MessageType::SCHEDULE_ACTUATOR, data, length)) {
    const NowManager::ScheduleActuatorMsg* msg =
        reinterpret_cast<const NowManager::ScheduleActuatorMsg*>(data);

    if (verifyCRC8(*msg)) relay.schedule(msg->offset, msg->duration);

  } else if (NowManager::validateMessage(NowManager::MessageType::PING, data,
                                         length)) {
    now.sendActuatorStateMsg(relay.getState());
  }
}

void onSyncReceivedCallback(const uint8_t* mac, const uint8_t* data,
                            int length) {
  if (NowManager::validateMessage(NowManager::MessageType::SYNC_BROADCAST, data,
                                  length)) {
    const NowManager::SyncBroadcastMsg* msg =
        reinterpret_cast<const NowManager::SyncBroadcastMsg*>(data);

    if (verifyCRC8(*msg)) {
      // Registrar nuevo Master Peer y enviar mensaje de registro
      if (now.registerMasterPeer(mac) && now.sendRegistrationMsg()) {
        if (now.sendRegistrationMsg()) {
          now.unsuscribeOnReceived();
          now.onReceived(onConfirmRegistrationReceivedCallback);
        }
      }
    }
  }
}

void onSendCallback(const uint8_t* mac, esp_now_send_status_t status) {
  if (now.isMasterMac(mac)) {
    if (status != ESP_NOW_SEND_SUCCESS)
      now.setIsMasterConnected(false);
    else
      now.setIsMasterConnected(true);
  }
}

void onConfirmRegistrationReceivedCallback(const uint8_t* mac,
                                           const uint8_t* data, int length) {
  if (NowManager::validateMessage(NowManager::MessageType::CONFIRM_REGISTRATION,
                                  data, length)) {
    if (config.saveMasterMacConfig(mac)) {
      ESP.restart();
    }
  }
}

void blinkLEDTask(void* parameter) {
  while (1) {
    led.set(true);
    vTaskDelay(pdMS_TO_TICKS(500));  // 500ms

    led.set(false);
    vTaskDelay(pdMS_TO_TICKS(500));  // 500ms
  }
}

void enterSyncMode() {
  if (!syncModeState) {
    disableDataTransfer();

    if (!now.reset()) ESP.restart();

    if (!now.registerBroadcastPeer()) return;

    now.onReceived(onSyncReceivedCallback);

    if (blinkLEDTaskHandler == NULL) {
      xTaskCreatePinnedToCore(blinkLEDTask, "Blink LED", 2048, NULL, 2,
                              &blinkLEDTaskHandler, 1);
    }

    syncModeTimeoutTimerHandler = xTimerCreate(
        "Sync Mode Timeout", pdMS_TO_TICKS(NowManager::SYNC_MODE_TIMEOUT),
        pdFALSE, (void*)0,
        syncModeTimeoutCallback);  // 30s

    if (syncModeTimeoutTimerHandler != NULL)
      xTimerStart(syncModeTimeoutTimerHandler, 0);

    syncModeState = true;
  }
}

void endSyncMode() {
  if (syncModeState) {
    // Detener el timer
    xTimerStop(syncModeTimeoutTimerHandler, 0);

    // Eliminar el timer y liberar memoria
    if (xTimerDelete(syncModeTimeoutTimerHandler, 0) == pdPASS)
      syncModeTimeoutTimerHandler =
          NULL;  // Reasignar a NULL para evitar usos indebidos

    if (blinkLEDTaskHandler != NULL) {
      vTaskDelete(blinkLEDTaskHandler);
      blinkLEDTaskHandler = NULL;
      led.set(false);
    }

    // Reiniciar ESP-NOW
    if (!now.reset()) ESP.restart();

    now.init();
    now.onReceived(onReceivedCallback);
    now.onSend(onSendCallback);
    enableDataTransfer;
    registerMasterNode();

    syncModeState = false;
  }
}

void onSimpleButtonPressCallback() {
  if (!syncModeState) {
    relay.cancelSchedule();
    relay.set(!relay.getState());
  } else
    endSyncMode();
}

void registerMasterNode() {
  uint8_t masterMac[6];
  config.copyMasterMac(masterMac);
  if (!now.registerMasterPeer(masterMac)) disableDataTransfer();
}

void sendActuatorStateTask(void* parameter) {
  while (1) {
    now.sendActuatorStateMsg(relay.getState());

    vTaskDelay(pdMS_TO_TICKS(5000));  // 5s
  }
}

void enableDataTransfer() {
  now.setDataTransfer(true);

  if (sendActuatorStateTaskHandler == NULL) {
    xTaskCreatePinnedToCore(sendActuatorStateTask, "Send ActuatorState", 2048,
                            NULL, 2, &sendActuatorStateTaskHandler, 1);
  }
}

void disableDataTransfer() {
  now.setDataTransfer(false);

  if (sendActuatorStateTaskHandler != NULL) {
    vTaskDelete(sendActuatorStateTaskHandler);
    sendActuatorStateTaskHandler = NULL;
  }
}

void onSetCallback() { now.sendActuatorStateMsg(relay.getState()); }
