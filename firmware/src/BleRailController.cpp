#include "BleRailController.h"

#include <NimBLEDevice.h>

class BleRailCharacteristicCallbacks : public NimBLECharacteristicCallbacks {
 public:
  explicit BleRailCharacteristicCallbacks(BleRailController& controller)
      : controller_(controller) {}

  void onRead(NimBLECharacteristic* characteristic, NimBLEConnInfo&) override {
    controller_.onCharacteristicRead(characteristic);
  }

  void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo&) override {
    controller_.onCharacteristicWrite(characteristic);
  }

 private:
  BleRailController& controller_;
};

class BleRailServerCallbacks : public NimBLEServerCallbacks {
 public:
  explicit BleRailServerCallbacks(BleRailController& controller)
      : controller_(controller) {}

  void onConnect(NimBLEServer*, NimBLEConnInfo&) override {
    controller_.onConnected();
  }

  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
    controller_.onDisconnected();
    NimBLEDevice::startAdvertising();
  }

 private:
  BleRailController& controller_;
};

BleRailController::BleRailController(BleRailControllerCallbacks callbacks)
    : callbacks_(callbacks) {}

void BleRailController::begin() {
  NimBLEDevice::init(kDeviceName);
  server_ = NimBLEDevice::createServer();
  serverCallbacks_ = new BleRailServerCallbacks(*this);
  server_->setCallbacks(serverCallbacks_);
  NimBLEService* service = server_->createService(kServiceUuid);

  const uint8_t capabilities[] = {
      kRailSpeedCount,
      kTurnoutCount,
  };
  const uint8_t initialValues[16] = {};

  capabilitiesCharacteristic_ =
      service->createCharacteristic(kCapabilitiesUuid, NIMBLE_PROPERTY::READ);
  capabilitiesCharacteristic_->setValue(capabilities, sizeof(capabilities));

  railSpeedCharacteristic_ = service->createCharacteristic(
      kRailSpeedUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR);
  railSpeedCharacteristic_->setValue(initialValues, kRailSpeedCount);

  turnoutCharacteristic_ = service->createCharacteristic(
      kTurnoutUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE_NR);
  turnoutCharacteristic_->setValue(initialValues, kTurnoutCount);

  characteristicCallbacks_ = new BleRailCharacteristicCallbacks(*this);
  railSpeedCharacteristic_->setCallbacks(characteristicCallbacks_);
  turnoutCharacteristic_->setCallbacks(characteristicCallbacks_);

  service->start();

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->enableScanResponse(true);
  advertising->start();
}

void BleRailController::onCharacteristicRead(NimBLECharacteristic* characteristic) {
  if (characteristic == railSpeedCharacteristic_) {
    if (callbacks_.readRailSpeed != nullptr) {
      const RailSpeedState state = callbacks_.readRailSpeed();
      characteristic->setValue(&state.value, sizeof(state.value));
    }
    return;
  }

  if (characteristic == turnoutCharacteristic_ && callbacks_.readTurnout != nullptr) {
    const TurnoutState state = callbacks_.readTurnout();
    const uint8_t values[] = {
        state.bits.turnout0Branch,
        state.bits.turnout1Branch,
    };
    characteristic->setValue(values, sizeof(values));
  }
}

void BleRailController::onCharacteristicWrite(NimBLECharacteristic* characteristic) {
  const std::string value = characteristic->getValue();

  if (characteristic == railSpeedCharacteristic_) {
    if (value.size() != kRailSpeedCount) {
      return;
    }

    RailSpeedState state{};
    state.value = static_cast<uint8_t>(value[0]);
    if (state.bits.power <= 100 && callbacks_.writeRailSpeed != nullptr) {
      callbacks_.writeRailSpeed(state);
    }
    return;
  }

  if (characteristic == turnoutCharacteristic_) {
    if (value.size() != kTurnoutCount) {
      return;
    }

    const uint8_t turnout0 = static_cast<uint8_t>(value[0]);
    const uint8_t turnout1 = static_cast<uint8_t>(value[1]);
    if (turnout0 > 1 || turnout1 > 1) {
      return;
    }

    TurnoutState state{};
    state.bits.turnout0Branch = turnout0;
    state.bits.turnout1Branch = turnout1;
    if (callbacks_.writeTurnout != nullptr) {
      callbacks_.writeTurnout(state);
    }
  }
}

void BleRailController::onConnected() {
  if (callbacks_.onConnected != nullptr) {
    callbacks_.onConnected();
  }
}

void BleRailController::onDisconnected() {
  if (callbacks_.onDisconnected != nullptr) {
    callbacks_.onDisconnected();
  }
}
