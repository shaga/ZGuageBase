#include <M5DinMeter.h>
#include "display.h"
#include "TurnoutController.h"
#include "TrainController.h"

extern TurnoutController turnout10;
extern TurnoutController turnout11;
extern int8_t power;
extern bool counterClockwise;
extern Cursor cursor;

void drawDirectionMark(bool clear) {
  auto& display = DinMeter.Display;
  const int color = cursor == Cursor::Speed ? BLACK : WHITE;
  const int backColor = cursor == Cursor::Speed ? WHITE : BLACK;
  constexpr int kCenterY = 22;
  constexpr int kRadius = 20;
  constexpr int kLineWidth = 4;
  constexpr int kLineLength = 70;
  constexpr int kBaseOffsetX = 4;
  constexpr int kOverLineBaseY = kCenterY - kRadius;
  constexpr int kUnderLineBaseY = kCenterY + kRadius - kLineWidth + 1;
  constexpr int kTriangleBaseY = kOverLineBaseY;
  constexpr int kTriangleSize = 12;

  if (clear) {
    display.fillRect(0, 0, 100, display.height()/3, backColor);
  }

  if (counterClockwise) {
    constexpr int kTriangleBaseX = kBaseOffsetX;
    display.fillRect(kBaseOffsetX + kTriangleSize, kOverLineBaseY, kLineLength - kTriangleSize, kLineWidth, color);
    display.fillRect(kBaseOffsetX, kUnderLineBaseY, kLineLength, kLineWidth, color);
    display.fillArc(kBaseOffsetX + kLineLength, kCenterY, kRadius - kLineWidth, kRadius, -90, 90, color);
    display.fillTriangle(kTriangleBaseX, kTriangleBaseY, kTriangleBaseX + kTriangleSize, kTriangleBaseY, kTriangleBaseX + kTriangleSize, kTriangleBaseY + kTriangleSize);
  } else {
    constexpr int kTriangleBaseX = kBaseOffsetX + kRadius + kLineLength;
    display.fillRect(kBaseOffsetX + kRadius, kOverLineBaseY, kLineLength - kTriangleSize, kLineWidth, color);
    display.fillRect(kBaseOffsetX + kRadius, kUnderLineBaseY, kLineLength, kLineWidth, color);
    display.fillArc(kBaseOffsetX + kRadius, kCenterY, kRadius - kLineWidth, kRadius, 90, 270, color);
    display.fillTriangle(kTriangleBaseX, kTriangleBaseY, kTriangleBaseX - kTriangleSize, kTriangleBaseY, kTriangleBaseX - kTriangleSize, kTriangleBaseY + kTriangleSize);
  }
}

void drawNearTurnoutDirection() {
  auto& display = DinMeter.Display;

  display.setTextColor(cursor == Cursor::NearTurnout ? BLACK : WHITE, cursor == Cursor::NearTurnout ? WHITE : BLACK);
  display.setTextDatum(middle_right);

  switch (turnout10.getDirection()) {
    case TurnoutController::Direction::Straight:
      display.drawString("STRAIGHT", display.width()-1, display.height() / 2);
      break;
    case TurnoutController::Direction::Branch:
      display.drawString("  BRANCH", display.width()-1, display.height() / 2);
      break;
  }
}

void drawFarTurnoutDirection() {
  auto& display = DinMeter.Display;

  display.setTextColor(cursor == Cursor::FarTurnout ? BLACK : WHITE, cursor == Cursor::FarTurnout ? WHITE : BLACK);
  display.setTextDatum(middle_right);

  switch (turnout11.getDirection()) {
    case TurnoutController::Direction::Straight:
      display.drawString("STRAIGHT", display.width()-1, display.height() * 5 / 6);
      break;
    case TurnoutController::Direction::Branch:
      display.drawString("  BRANCH", display.width()-1, display.height() * 5 /6);
      break;
  }
}

void drawPower() {
  auto& display = DinMeter.Display;

  display.setTextColor(cursor == Cursor::Speed ? BLACK : WHITE,
                       cursor == Cursor::Speed ? WHITE : BLACK);
  display.setTextDatum(middle_right);

  char label[5];
  sprintf(label, "%3d", power);

  display.drawString(label, display.width() - 1, display.height() / 6);
}

void drawCursor() {
  auto& display = DinMeter.Display;
  const int CursorOffset = 8;
  const int CursorWidth = 2;

  display.setTextDatum(middle_left);
  switch (cursor) {
    case Cursor::Speed:
      display.fillRect(0, 0, display.width(), display.height() / 3, WHITE);
      display.fillRect(0, display.height() / 3, display.width(), display.height() / 3, BLACK);
      display.fillRect(0, display.height() * 2 / 3, display.width(), display.height() / 3, BLACK);
      display.setTextColor(WHITE, BLACK);
      display.drawString("Near:", 0, display.height() / 2);
      display.drawString("Far :", 0, display.height() * 5 / 6);
      break;
    case Cursor::NearTurnout:;
      display.fillRect(0, display.height() / 3, display.width(), display.height() / 3, WHITE);
      display.setTextColor(BLACK, WHITE);
      display.drawString("Near:", 0, display.height() / 2);
      display.fillRect(0, 0, display.width(), display.height() / 3, BLACK);
      display.fillRect(0, display.height() * 2 / 3, display.width(), display.height() / 3, BLACK);
      display.setTextColor(WHITE, BLACK);
      display.drawString("Far :", 0, display.height() * 5 / 6);
      break;
    case Cursor::FarTurnout:
      display.fillRect(0, display.height() * 2 / 3, display.width(), display.height() / 3, WHITE);
      display.setTextColor(BLACK, WHITE);
      display.drawString("Far :", 0, display.height() * 5 / 6);
      display.fillRect(0, 0, display.width(), display.height() / 3, BLACK);
      display.fillRect(0, display.height() / 3, display.width(), display.height() / 3, BLACK);
      display.setTextColor(WHITE, BLACK);
      display.drawString("Near:", 0, display.height() / 2);
      break;
  }

  drawDirectionMark();
  drawPower();
  drawNearTurnoutDirection();
  drawFarTurnoutDirection();
}
