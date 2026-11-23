#include "BleCentral.h"

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLERemoteCharacteristic.h>
#include <atomic>

namespace BleCentral {
namespace {
constexpr char serviceUuid[] = "8a5e1020-f39d-42a4-ad5c-34f68745e643";
constexpr char capabilitiesUuid[] = "8a5e1021-f39d-42a4-ad5c-34f68745e643";
constexpr char speedUuid[] = "8a5e1022-f39d-42a4-ad5c-34f68745e643";
constexpr char turnoutUuid[] = "8a5e1023-f39d-42a4-ad5c-34f68745e643";
std::atomic<State> current{State::Starting};
std::atomic<bool> linked{false};
// -1 means no user command yet: connecting alone must not move the turnouts.
std::atomic<int> requestedTurnouts{-1};
std::atomic<int> requestedSpeed{0};
Device discovered[8]{};
int discoveredCount = 0;
portMUX_TYPE deviceMutex = portMUX_INITIALIZER_UNLOCKED;
std::atomic<int> selected{-1};

int remember(BLEAdvertisedDevice& device) {
  const String address = device.getAddress().toString();
  const String name = device.haveName() ? device.getName() : String("ZGuageBase");
  portENTER_CRITICAL(&deviceMutex);
  int index = 0;
  while (index < discoveredCount && strcmp(discovered[index].address, address.c_str()) != 0) ++index;
  if (index < 8) {
    snprintf(discovered[index].address, sizeof(discovered[index].address), "%s", address.c_str());
    snprintf(discovered[index].name, sizeof(discovered[index].name), "%.40s", name.c_str());
    if (index == discoveredCount) ++discoveredCount;
  }
  portEXIT_CRITICAL(&deviceMutex);
  return index < 8 ? index : -1;
}

void setState(State value) {
  current.store(value);
  Serial.printf("[BLE] %s\n", statusText(value));
}

class Callbacks : public BLEClientCallbacks {
  void onConnect(BLEClient*) override { linked.store(true); }
  void onDisconnect(BLEClient*) override {
    linked.store(false);
    requestedSpeed.store(0);
  }
} callbacks;

bool checkProtocol(BLEClient* client) {
  // Refresh discovery on every connection, including after a peripheral reboot.
  client->getServices();
  auto* service = client->getService(serviceUuid);
  if (!service) return false;
  auto* capabilities = service->getCharacteristic(capabilitiesUuid);
  auto* speed = service->getCharacteristic(speedUuid);
  auto* turnout = service->getCharacteristic(turnoutUuid);
  if (!capabilities || !capabilities->canRead() || !speed || !turnout ||
      !speed->canWriteNoResponse() || !turnout->canWriteNoResponse()) return false;
  const String counts = capabilities->readValue();
  return linked.load() && counts.length() == 2 && counts[0] == 1 && counts[1] == 2;
}

void run(void*) {
  if (!BLEDevice::init("Tab5-ZGauge")) {
    setState(State::Failed);
    vTaskDelete(nullptr);
    return;
  }
  auto* client = BLEDevice::createClient();
  auto* scan = BLEDevice::getScan();
  if (!client || !scan) {
    setState(State::Failed);
    vTaskDelete(nullptr);
    return;
  }
  client->setClientCallbacks(&callbacks);
  scan->setActiveScan(true);
  for (;;) {
    setState(State::Scanning);
    auto* results = scan->start(5, false);
    bool found = false;
    if (results) {
      for (int i = 0; i < results->getCount(); ++i) {
        auto device = results->getDevice(i);
        if (!device.isAdvertisingService(BLEUUID(serviceUuid))) continue;
        const int index = remember(device);
        if (index < 0 || selected.load() != index) continue;
        found = true;
        setState(State::Connecting);
        Serial.printf("[BLE] peer %s\n", device.getAddress().toString().c_str());
        if (client->connectTimeout(&device, 10000)) {
          setState(State::Checking);
          if (checkProtocol(client) && selected.load() == index) {
            requestedSpeed.store(0);
            setState(State::Connected);
          } else {
            setState(linked.load() ? State::Incompatible : State::Retrying);
            client->disconnect();
          }
        } else {
          setState(State::Retrying);
        }
        break;
      }
    }
    scan->clearResults();
    // Only this task accesses GATT. Coalesce rapid taps into the latest pair.
    int lastSent = -1;
    int lastSpeedSent = -1;
    BLERemoteCharacteristic* turnout = nullptr;
    BLERemoteCharacteristic* speed = nullptr;
    if (current.load() == State::Connected && linked.load()) {
      auto* service = client->getService(serviceUuid);
      if (service) {
        turnout = service->getCharacteristic(turnoutUuid);
        speed = service->getCharacteristic(speedUuid);
      }
    }
    while (linked.load()) {
      if (selected.load() < 0) {
        client->disconnect();
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
      }
      const int speedValue = requestedSpeed.load();
      if (speed && speedValue != lastSpeedSent) {
        uint8_t value = static_cast<uint8_t>(speedValue);
        if (speed->writeValue(&value, 1, false)) {
          lastSpeedSent = speedValue;
          Serial.printf("[BLE] speed command sent: power=%u ccw=%u\n",
                        value & 0x7f, value >> 7);
        } else {
          Serial.println("[BLE] speed write failed; retrying");
          vTaskDelay(pdMS_TO_TICKS(500));
        }
      }
      const int requested = requestedTurnouts.load();
      if (turnout && requested >= 0 && requested != lastSent) {
        // Wire format is two bytes, not a bit mask: near/lower, far/upper.
        uint8_t values[] = {static_cast<uint8_t>(requested & 1),
                            static_cast<uint8_t>((requested >> 1) & 1)};
        if (turnout->writeValue(values, sizeof(values), false)) {
          lastSent = requested;
          Serial.printf("[BLE] turnout command sent: lower=%u upper=%u\n",
                        values[0], values[1]);
        } else {
          Serial.println("[BLE] turnout write failed; retrying");
          vTaskDelay(pdMS_TO_TICKS(500));
        }
      }
      vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (current.load() == State::Connected || !found) setState(State::Retrying);
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
}

void begin() {
  static bool started = false;
  if (started) return;
  started = true;
  if (xTaskCreate(run, "ble-central", 8192, nullptr, 1, nullptr) != pdPASS) {
    setState(State::Failed);
  }
}

State state() { return current.load(); }

int devices(Device* output, int capacity) {
  portENTER_CRITICAL(&deviceMutex);
  const int count = capacity < discoveredCount ? capacity : discoveredCount;
  for (int i = 0; i < count; ++i) output[i] = discovered[i];
  portEXIT_CRITICAL(&deviceMutex);
  return count;
}

int selectedDevice() { return selected.load(); }

bool selectDevice(int index) {
  portENTER_CRITICAL(&deviceMutex);
  const bool valid = index >= 0 && index < discoveredCount;
  portEXIT_CRITICAL(&deviceMutex);
  if (!valid || linked.load() || selected.load() >= 0) return false;
  requestedTurnouts.store(-1);
  requestedSpeed.store(0);
  selected.store(index);
  return true;
}

void returnToSelection() {
  requestedSpeed.store(0);
  selected.store(-1);
}

int speedCommand() { return requestedSpeed.load(); }

bool setSpeed(int power, bool counterClockwise) {
  if (!linked.load() || current.load() != State::Connected ||
      power < 0 || power > 100 || power % 2 != 0) return false;
  const int previous = requestedSpeed.load();
  if ((previous & 0x7f) != 0 && ((previous & 0x80) != 0) != counterClockwise) return false;
  requestedSpeed.store(power | (counterClockwise ? 0x80 : 0));
  return true;
}

void setTurnouts(bool lowerBranch, bool upperBranch) {
  requestedTurnouts.store((lowerBranch ? 1 : 0) | (upperBranch ? 2 : 0));
}

const char* statusText(State value) {
  switch (value) {
    case State::Starting: return "BLE: starting";
    case State::Scanning: return "BLE: scanning";
    case State::Connecting: return "BLE: connecting";
    case State::Checking: return "BLE: checking service";
    case State::Connected: return "BLE: connected - ZGuageBase";
    case State::Retrying: return "BLE: waiting / retrying";
    case State::Incompatible: return "BLE: incompatible service";
    case State::Failed: return "BLE: init failed - see serial";
  }
  return "BLE: unknown";
}
}
