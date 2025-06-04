#include "NowManager.hpp"

#include <WiFi.h>

#include "Utils.hpp"

bool NowManager::init() {
  if (esp_now_init() != ESP_OK) return false;

  return true;
}

bool NowManager::stop() {
  if (reset() && esp_now_deinit() != ESP_OK) return false;

  return true;
}

bool NowManager::reset() {
  // Eliminar masterPeer si existe
  if (_isMasterPeerRegistered) {
    if (esp_now_del_peer(_masterMac) != ESP_OK) return false;

    _isMasterPeerRegistered = false;
  }

  // Eliminar el broadcast peer si existe
  if (_isBroadcastPeerRegistered) {
    if (esp_now_del_peer(_broadcastMac) != ESP_OK) return false;

    _isBroadcastPeerRegistered = false;
  }

  // Desregistrar callbacks
  if (esp_now_unregister_recv_cb() != ESP_OK ||
      esp_now_unregister_send_cb() != ESP_OK)
    return false;

  return true;
}

void NowManager::onSend(esp_now_send_cb_t callback) {
  esp_now_register_send_cb(callback);
}

void NowManager::unsuscribeOnSend() { esp_now_unregister_send_cb(); }

void NowManager::onReceived(esp_now_recv_cb_t callback) {
  esp_now_register_recv_cb(callback);
}

void NowManager::unsuscribeOnReceived() { esp_now_unregister_recv_cb(); }

bool NowManager::sendRegistrationMsg() {
  NowManager::RegistrationMsg msg;
  msg.nodeType = NODE_TYPE;
  msg.firmwareVersion[0] = FIRMWARE_VERSION[0];
  msg.firmwareVersion[1] = FIRMWARE_VERSION[1];
  msg.firmwareVersion[2] = FIRMWARE_VERSION[2];

  // Generate CRC8
  addCRC8(msg);

  return esp_now_send(_masterMac, (uint8_t*)&msg, sizeof(msg)) == ESP_OK;
}

bool NowManager::sendActuatorStateMsg(const bool state) {
  if (!_isDataTransferEnabled) return false;

  NowManager::ActuatorStateMsg msg;
  msg.state = state;

  // Generate CRC8
  addCRC8(msg);

  return esp_now_send(_masterMac, (uint8_t*)&msg, sizeof(msg)) == ESP_OK;
}

void NowManager::setDataTransfer(const bool state) {
  _isDataTransferEnabled = state;
}

void NowManager::setIsMasterConnected(const bool state) {
  _isMasterConnected = state;
}

bool NowManager::validateMessage(MessageType expectedType, const uint8_t* data,
                                 size_t length) {
  // Evitar mensajes vacíos
  if (length < 1) return false;

  auto msgType = static_cast<MessageType>(data[0]);
  return (msgType == expectedType) && (length == _getMessageSize(expectedType));
}

size_t NowManager::_getMessageSize(MessageType type) {
  switch (type) {
    case MessageType::SYNC_BROADCAST:
      return sizeof(SyncBroadcastMsg);

    case MessageType::REGISTRATION:
      return sizeof(RegistrationMsg);

    case MessageType::CONFIRM_REGISTRATION:
      return sizeof(ConfirmRegistrationMsg);

    case MessageType::SET_ACTUATOR:
      return sizeof(SetActuatorMsg);

    case MessageType::SCHEDULE_ACTUATOR:
      return sizeof(ScheduleActuatorMsg);

    case MessageType::PING:
      return sizeof(PingMsg);

    default:
      return 0;  // Tipo desconocido
  }
}

bool NowManager::registerMasterPeer(const uint8_t* masterMac) {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, masterMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;

  memcpy(_masterMac, masterMac, 6);

  _isMasterPeerRegistered = true;

  return true;
}

bool NowManager::registerBroadcastPeer() {
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, _broadcastMac, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) return false;

  _isBroadcastPeerRegistered = true;
  return true;
}

// bool NowManager::sendDataMsg(const float temp, const float hum) {
//   NowManager::TemperatureHumidityMsg msg;
//   msg.temp = temp;
//   msg.hum = hum;

//   // Generate CRC8
//   addCRC8(msg);

//   return esp_now_send(_masterMac, (uint8_t*)&msg, sizeof(msg)) == ESP_OK;
// }

bool NowManager::isMasterMac(const uint8_t* mac) {
  return memcmp(_masterMac, mac, 6) == 0;
}
