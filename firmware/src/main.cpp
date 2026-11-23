#include <Motoron.h>
#include <M5DinMeter.h>
#include "BleRailController.h"
#include "display.h"
#include "TurnoutController.h"
#include "TrainController.h"

MotoronI2C motoron10(0x10);
MotoronI2C motoron11(0x11);

TurnoutController turnout10(motoron10, 1, 2);
TurnoutController turnout11(motoron11, 2, 3);
TrainController trainOutside(motoron10, 3);
TrainController trainInside(motoron11, 1, true);

int oldEnc = 0;
int8_t power = 0;
bool counterClockwise = true;
Cursor cursor = Cursor::Speed;

namespace {
constexpr uint32_t kTrainRefreshMs = 200;
SemaphoreHandle_t motoronMutex = nullptr;
QueueHandle_t bleConnectionEvents = nullptr;
QueueHandle_t bleRailSpeedEvents = nullptr;
QueueHandle_t bleTurnoutEvents = nullptr;
bool bleConnected = false;

void onBleConnected() {
  const bool connected = true;
  xQueueOverwrite(bleConnectionEvents, &connected);
}

void onBleDisconnected() {
  const bool connected = false;
  xQueueOverwrite(bleConnectionEvents, &connected);
}

void onBleRailSpeedWrite(RailSpeedState state) {
  xQueueOverwrite(bleRailSpeedEvents, &state);
}

void onBleTurnoutWrite(TurnoutState state) {
  xQueueOverwrite(bleTurnoutEvents, &state);
}

const BleRailControllerCallbacks bleCallbacks = {
    onBleConnected, onBleDisconnected, nullptr, nullptr,
    onBleRailSpeedWrite, onBleTurnoutWrite};
BleRailController bleRailController(bleCallbacks);

void handleBleConnectionEvent(bool connected) {
  xSemaphoreTake(motoronMutex, portMAX_DELAY);
  power = 0;
  trainOutside.setPower(0);
  trainInside.setPower(0);
  xSemaphoreGive(motoronMutex);

  bleConnected = connected;
  oldEnc = DinMeter.Encoder.read();
  auto& display = DinMeter.Display;
  display.fillScreen(TFT_BLACK);
  if (connected) {
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setTextDatum(middle_center);
    display.drawString("BLE", display.width() / 2, display.height() / 2 - 22);
    display.drawString("connected", display.width() / 2, display.height() / 2 + 22);
  } else {
    xQueueReset(bleRailSpeedEvents);
    xQueueReset(bleTurnoutEvents);
    drawCursor();
  }
}

void handleBleRailSpeedWrite(RailSpeedState state) {
  xSemaphoreTake(motoronMutex, portMAX_DELAY);
  power = state.bits.power;
  counterClockwise = state.bits.counterClockwise;
  const int16_t motorPower = counterClockwise ? power : -power;
  trainOutside.setPower(motorPower);
  trainInside.setPower(motorPower);
  xSemaphoreGive(motoronMutex);
}

void handleBleTurnoutWrite(TurnoutState state) {
  using Direction = TurnoutController::Direction;
  const Direction nearDirection = state.bits.turnout0Branch ? Direction::Branch : Direction::Straight;
  const Direction farDirection = state.bits.turnout1Branch ? Direction::Branch : Direction::Straight;

  xSemaphoreTake(motoronMutex, portMAX_DELAY);
  turnout10.switchStart(nearDirection);
  turnout11.switchStart(farDirection);
  xSemaphoreGive(motoronMutex);
  TurnoutController::delaySwitch();
  xSemaphoreTake(motoronMutex, portMAX_DELAY);
  turnout10.switchEnd();
  turnout11.switchEnd();
  xSemaphoreGive(motoronMutex);
}

void trainRefreshTask(void*) {
  for (;;) {
    xSemaphoreTake(motoronMutex, portMAX_DELAY);
    if (power != 0) {
      const int16_t motorPower = counterClockwise ? power : -power;
      trainOutside.setPower(motorPower);
      trainInside.setPower(motorPower);
      vTaskDelay(pdMS_TO_TICKS(90));
      trainOutside.setPower(motorPower + (motorPower < 0 ? 1 : -1));
      trainInside.setPower(motorPower + (motorPower < 0 ? 1 : -1));
    }
    xSemaphoreGive(motoronMutex);
    vTaskDelay(pdMS_TO_TICKS(kTrainRefreshMs));
  }
}

}

void initializeMotoron(MotoronI2C& motoron) {
  motoron.reinitialize();
  delay(5);
  motoron.disableCrc();
  motoron.clearResetFlag();
}


void setup() {
  auto cfg = M5.config();
  DinMeter.begin(cfg, true);
  oldEnc = DinMeter.Encoder.read();

  auto& display = DinMeter.Display;
  display.setRotation(1);
  display.setTextDatum(middle_center);
  display.setTextSize(3);
  display.drawString("initialize...", display.width() / 2, display.height() / 2);

  Wire.begin();
  initializeMotoron(motoron10);
  initializeMotoron(motoron11);
  trainOutside.begin();
  trainInside.begin();
  turnout10.begin();
  turnout11.begin();


  motoronMutex = xSemaphoreCreateMutex();
  if (motoronMutex == nullptr) {
    display.drawString("Fail: mutext", display.width() / 2, display.height() / 2);
    return;
  }
  bleConnectionEvents = xQueueCreate(1, sizeof(bool));
  bleRailSpeedEvents = xQueueCreate(1, sizeof(RailSpeedState));
  bleTurnoutEvents = xQueueCreate(1, sizeof(TurnoutState));
  if (bleConnectionEvents == nullptr || bleRailSpeedEvents == nullptr || bleTurnoutEvents == nullptr) {
    display.drawString("Fail: queue", display.width() / 2, display.height() / 2);
    return;
  }
  xTaskCreate(trainRefreshTask, "TrainRefresh", 2048, nullptr, 1, nullptr);


  display.fillScreen(TFT_BLACK);

  drawCursor();
  bleRailController.begin();
}

void loop() {
  if (motoronMutex == nullptr || bleConnectionEvents == nullptr ||
      bleRailSpeedEvents == nullptr || bleTurnoutEvents == nullptr) return;

  DinMeter.update();
  int currentEnc = DinMeter.Encoder.read();
  bool connected;
  if (xQueueReceive(bleConnectionEvents, &connected, 0) == pdTRUE) {
    handleBleConnectionEvent(connected);
    delay(5);
    return;
  }
  if (bleConnected) {
    RailSpeedState railSpeed;
    if (xQueueReceive(bleRailSpeedEvents, &railSpeed, 0) == pdTRUE) {
      handleBleRailSpeedWrite(railSpeed);
    }
    TurnoutState turnout;
    if (xQueueReceive(bleTurnoutEvents, &turnout, 0) == pdTRUE) {
      handleBleTurnoutWrite(turnout);
    }
    oldEnc = currentEnc;
    delay(5);
    return;
  }
  if (M5.BtnA.wasClicked()) {
    switch(cursor) {
      case Cursor::Speed:
        cursor = Cursor::NearTurnout;
        break;
      case Cursor::NearTurnout:
        cursor = Cursor::FarTurnout;
        break;
      case Cursor::FarTurnout:
        cursor = Cursor::Speed;
        break;
    }
    drawCursor();
  } else if (M5.BtnA.wasHold()) {
    switch (cursor) {
      case Cursor::Speed:
        xSemaphoreTake(motoronMutex, portMAX_DELAY);
        if (power == 0) {
          counterClockwise = !counterClockwise;
          xSemaphoreGive(motoronMutex);
          drawDirectionMark(true);
        } else {
          power = 0;
          trainOutside.setPower(0);
          trainInside.setPower(0);
          xSemaphoreGive(motoronMutex);
          drawPower();
        }
        break;
      case Cursor::NearTurnout:
        xSemaphoreTake(motoronMutex, portMAX_DELAY);
        turnout10.switchStart();
        xSemaphoreGive(motoronMutex);
        TurnoutController::delaySwitch();
        xSemaphoreTake(motoronMutex, portMAX_DELAY);
        turnout10.switchEnd();
        xSemaphoreGive(motoronMutex);
        drawNearTurnoutDirection();
        break;
      case Cursor::FarTurnout:
        xSemaphoreTake(motoronMutex, portMAX_DELAY);
        turnout11.switchStart();
        xSemaphoreGive(motoronMutex);
        TurnoutController::delaySwitch();
        xSemaphoreTake(motoronMutex, portMAX_DELAY);
        turnout11.switchEnd();
        xSemaphoreGive(motoronMutex);
        drawFarTurnoutDirection();
        break;
    }
  } else if (cursor == Cursor::Speed && currentEnc != oldEnc) {
    const int encoderDelta = oldEnc - currentEnc;
    xSemaphoreTake(motoronMutex, portMAX_DELAY);
    const int nextPower = power + encoderDelta;
    power = nextPower > 100 ? 100 : (nextPower < 0 ? 0 : nextPower);
    if (power == 0) {
      trainOutside.setPower(0);
      trainInside.setPower(0);
    }
    xSemaphoreGive(motoronMutex);
    drawPower();
  }

  oldEnc = currentEnc;
}
