#include <Arduino.h>
#include <M5Unified.h>
#include "AppUi.h"
#include "BleCentral.h"

void setup() {
  M5.begin(M5.config());
  Serial.begin(115200);
  if (AppUi::begin()) BleCentral::begin();
}

void loop() {
  AppUi::update();
  delay(20);
}
