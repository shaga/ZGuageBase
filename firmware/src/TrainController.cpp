#include "TrainController.h"

TrainController::TrainController(MotoronI2C& driver, uint8_t port, bool inbert)
    : driver_(driver), port_(port), ivert_(inbert) {}

void TrainController::begin() {
  setPower(0);
}

void TrainController::setPower(int16_t power) {
  if (power > 100) {
    power = 100;
  } else if (power < -100) {
    power = -100;
  }

  power_ = power;
  int16_t pwm = static_cast<int32_t>(power_) * kMaxPwm / 100;
  if (ivert_) {
    pwm = -pwm;
  }
  driver_.setSpeedNow(port_, pwm);
}

void TrainController::stop() {
  setPower(0);
}
