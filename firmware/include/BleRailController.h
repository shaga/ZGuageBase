#pragma once

#include <cstdint>

class NimBLECharacteristic;
class NimBLEServer;
class BleRailCharacteristicCallbacks;
class BleRailServerCallbacks;

union RailSpeedState {
  uint8_t value;

  struct {
    uint8_t power : 7;
    uint8_t counterClockwise : 1;
  } bits;
};

union TurnoutState {
  uint8_t value;

  struct {
    uint8_t turnout0Branch : 1;
    uint8_t turnout1Branch : 1;
  } bits;
};

struct BleRailControllerCallbacks {
  void (*onConnected)() = nullptr;
  void (*onDisconnected)() = nullptr;

  RailSpeedState (*readRailSpeed)() = nullptr;
  TurnoutState (*readTurnout)() = nullptr;

  void (*writeRailSpeed)(RailSpeedState state) = nullptr;
  void (*writeTurnout)(TurnoutState state) = nullptr;
};

class BleRailController {
 public:
  explicit BleRailController(BleRailControllerCallbacks callbacks = {});

  void begin();

 private:
  friend class BleRailCharacteristicCallbacks;
  friend class BleRailServerCallbacks;

  inline static constexpr char kDeviceName[] = "ZGuageBase";
  inline static constexpr char kServiceUuid[] = "8a5e1020-f39d-42a4-ad5c-34f68745e643";
  inline static constexpr char kCapabilitiesUuid[] = "8a5e1021-f39d-42a4-ad5c-34f68745e643";
  inline static constexpr char kRailSpeedUuid[] = "8a5e1022-f39d-42a4-ad5c-34f68745e643";
  inline static constexpr char kTurnoutUuid[] = "8a5e1023-f39d-42a4-ad5c-34f68745e643";
  inline static constexpr uint8_t kRailSpeedCount = 1;
  inline static constexpr uint8_t kTurnoutCount = 2;

  void onCharacteristicRead(NimBLECharacteristic* characteristic);
  void onCharacteristicWrite(NimBLECharacteristic* characteristic);
  void onConnected();
  void onDisconnected();

  const BleRailControllerCallbacks callbacks_;
  NimBLEServer* server_ = nullptr;
  NimBLECharacteristic* capabilitiesCharacteristic_ = nullptr;
  NimBLECharacteristic* railSpeedCharacteristic_ = nullptr;
  NimBLECharacteristic* turnoutCharacteristic_ = nullptr;
  BleRailCharacteristicCallbacks* characteristicCallbacks_ = nullptr;
  BleRailServerCallbacks* serverCallbacks_ = nullptr;
};
