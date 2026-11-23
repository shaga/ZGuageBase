#include <Arduino.h>
#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <esp32-hal-hosted.h>

#if !defined(CONFIG_ESP_HOSTED_ENABLE_BT_NIMBLE)
#error "This diagnostic requires ESP-Hosted NimBLE support."
#endif
#if !defined(BOARD_HAS_SDIO_ESP_HOSTED)
#error "Select the Tab5 board variant with its ESP-Hosted SDIO pins."
#endif
#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL) || !CONFIG_BT_NIMBLE_ROLE_CENTRAL
#error "The BLE central role must be enabled in the SDK."
#endif

void setup() {
  M5.begin(M5.config());
  Serial.begin(115200);
  delay(1500);
  M5.Display.setTextSize(2);
  M5.Display.println("BLE environment check");
  Serial.println("Initializing Tab5 ESP-Hosted BLE...");
  if (!BLEDevice::init("Tab5-BLE-check")) {
    M5.Display.println("BLE init failed; see serial log");
    Serial.println("BLE init failed. Check the C6 firmware and SDIO log.");
    return;
  }
  // Exercise central/client support without connecting or writing commands.
  BLEClient* client = BLEDevice::createClient();
  if (!client) {
    M5.Display.println("BLE client creation failed");
    return;
  }
  M5.Display.println("Scanning for 5 seconds...");
  BLEScan* scan = BLEDevice::getScan();
  scan->setActiveScan(true);
  BLEScanResults* results = scan->start(5, false);
  if (!results) {
    M5.Display.println("Scan failed; see serial log");
    return;
  }
  Serial.printf("Found %d BLE devices\n", results->getCount());
  M5.Display.printf("Found %d BLE devices\n", results->getCount());
  const BLEUUID service("8a5e1020-f39d-42a4-ad5c-34f68745e643");
  for (int i = 0; i < results->getCount(); ++i) {
    BLEAdvertisedDevice device = results->getDevice(i);
    Serial.println(device.toString().c_str());
    if (device.haveServiceUUID() && device.isAdvertisingService(service)) {
      M5.Display.println("ZGuageBase service found");
      Serial.println("ZGuageBase service found");
    }
  }
  scan->clearResults();
  M5.Display.println("Check complete");
}

void loop() {
  M5.update();
  delay(20);
}
