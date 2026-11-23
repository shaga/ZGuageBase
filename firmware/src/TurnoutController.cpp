#include "TurnoutController.h"

namespace {
constexpr int16_t kFullSpeed = 800;
constexpr uint32_t kPulseMs = 100;
}

void TurnoutController::delaySwitch() {
  vTaskDelay(pdMS_TO_TICKS(kPulseMs));
}

TurnoutController::TurnoutController(MotoronI2C& driver, uint8_t port1, uint8_t port2)
    : driver_(driver), port1_(port1), port2_(port2) {}

void TurnoutController::begin() {
  switchStart(Direction::Straight);
  delaySwitch();
  switchEnd();
}

void TurnoutController::switchTo(Direction direction) {
  if (direction == Direction::Unknown || direction == direction_) {
    return;
  }
  switchStart(direction);
  delaySwitch();
  switchEnd();
}

void TurnoutController::switchStart(Direction direction) {
  if (direction == Direction::Invert) {
    direction = direction_ == Direction::Straight ? Direction::Branch
                                                   : Direction::Straight;
  } else if (direction == Direction::Unknown || direction == direction_) {
    return;
  }
  pendingDirection_ = direction;
  const int16_t power = direction == Direction::Straight ? kFullSpeed : -kFullSpeed;
  drive(power);
}

void TurnoutController::switchEnd() {
  drive(0);
  if (pendingDirection_ != Direction::Unknown) {
    direction_ = pendingDirection_;
    pendingDirection_ = Direction::Unknown;
  }
}

void TurnoutController::drive(int16_t power) {
  driver_.setSpeedNow(port1_, power);
  driver_.setSpeedNow(port2_, power);
}
