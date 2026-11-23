#pragma once

#include <Arduino.h>
#include <Motoron.h>

// 同じMotoronに接続された2つのポイントを一組として駆動する。
// driverはこのオブジェクトより長く生存すること。
class TurnoutController {
 public:
  // 直進は正転、分岐は逆転で駆動する。
  enum class Direction { Unknown, Straight, Branch, Invert };

  TurnoutController(MotoronI2C& driver, uint8_t port1, uint8_t port2);

  // Motoron初期化後に呼ぶ。必ず直進へ切り替え、通電停止まで待つ。
  void begin();

  // 100%出力で駆動し、200ms待機して停止してから戻る。
  void switchTo(Direction direction);

  void switchStart(Direction direction = Direction::Invert);
  void switchEnd();

  static void delaySwitch();

  // 最後に切替を完了した方向。通電中などの状態ではない。
  // begin()後の初期状態はStraight。
 Direction getDirection() const { return direction_; }

 private:
  void drive(int16_t power);

  MotoronI2C& driver_;
  const uint8_t port1_;
  const uint8_t port2_;
  volatile Direction direction_ = Direction::Unknown;
  Direction pendingDirection_ = Direction::Unknown;
};
