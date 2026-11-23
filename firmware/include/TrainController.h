#pragma once

#include <Arduino.h>
#include <Motoron.h>

// 1ポートに接続されたレールへの給電を制御する。
// driverはこのオブジェクトより長く生存すること。
class TrainController {
 public:
  // 1クリックの変化量。既定値は最大出力800の5%。1〜800に制限する。
  static constexpr int16_t kMaxPwm = 800;

  TrainController(MotoronI2C& driver, uint8_t port, bool inbert = false);

  // Motoron初期化後に呼び、出力をゼロにする。
  void begin();

  // 右回転は正、左回転は負のクリック数を渡す。
  // ゼロをまたぐ場合はゼロで止め、余ったクリック数は捨てる。
  void setPower(int16_t power);
  void stop();

  // 最後に指令したPWM出力。-800〜800。実際の車速ではない。
  int16_t getPower() const { return power_; }

 private:
  MotoronI2C& driver_;
  const uint8_t port_;
  bool ivert_ = false;
  int16_t power_ = 0;
};
