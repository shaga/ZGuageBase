#include "ControlScreen.h"
#include "BleCentral.h"
#include <math.h>
using namespace Ui;
namespace {
constexpr int speedStep = 2;
constexpr int maximumSpeed = 100;
constexpr int upperSwitchY = 36;
constexpr int lowerSwitchY = 500;
constexpr float branchStartX = 280;
constexpr float branchEndX = 440;

constexpr Rect plusButton{520, 600, 150, 130};
constexpr Rect minusButton{520, 960, 150, 130};
constexpr Rect stopButton{0, 1150, 720, 130};
constexpr Rect directionButton{220, 220, 280, 110};
}

void ControlScreen::oval(float left, float top, float right, float bottom,
          float width, uint16_t color) {
  // Half the track height makes each end a semicircle.
  const float radius = (bottom - top) / 2;
  const Point centers[] = {{right - radius, top + radius},
                           {right - radius, bottom - radius},
                           {left + radius, bottom - radius},
                           {left + radius, top + radius}};
  Point previous = {left + radius, top};
  for (int corner = 0; corner < 4; ++corner) {
    for (int step = 0; step <= 40; ++step) {
      const float angle = (-90 + corner * 90 + step * 90.0f / 40) * PI / 180;
      const Point next = {centers[corner].x + radius * cosf(angle),
                          centers[corner].y + radius * sinf(angle)};
      const bool cutStraight = step == 0 &&
          ((corner == 0 && upperBranch) || (corner == 2 && lowerBranch));
      if (cutStraight) {
        // The gap spans the same X positions as the diagonal's two junctions.
        const float y = corner == 0 ? top : bottom;
        ui_.stroke({left + radius, y}, {branchStartX, y}, width, color);
        ui_.stroke({branchEndX, y}, {right - radius, y}, width, color);
      } else {
        ui_.stroke(previous, next, width, color);
      }
      previous = next;
    }
  }
  ui_.stroke(previous, {left + radius, top}, width, color);
}

void ControlScreen::crossover(float startY, float endY, float width, uint16_t color) {
  // A diagonal branch joins the loops when the corresponding switch is selected.
  // Viewed left to right, decreasing Y branches left; increasing Y branches right.
  ui_.stroke({branchStartX, startY}, {branchEndX, endY}, width, color);
}

void ControlScreen::drawSwitch(int centerY, bool branch) {
  constexpr uint16_t straightColor = 0x4EBA;
  const uint16_t selectedColor = branch ? pointColor : straightColor;
  ui_.canvas().fillRect(lroundf(140 * ui_.scaleX()), lroundf((centerY - 30) * ui_.scaleY()),
                   lroundf(440 * ui_.scaleX()), lroundf(60 * ui_.scaleY()), background);
  ui_.label("straight", 160, centerY - 8, branch ? railColor : straightColor);
  ui_.label("branch", 450, centerY - 8, branch ? pointColor : railColor);
  ui_.canvas().fillRoundRect(lroundf(300 * ui_.scaleX()), lroundf((centerY - 24) * ui_.scaleY()),
                        lroundf(120 * ui_.scaleX()), lroundf(48 * ui_.scaleY()),
                        lroundf(24 * fminf(ui_.scaleX(), ui_.scaleY())), selectedColor);
  ui_.canvas().fillCircle(lroundf((branch ? 396 : 324) * ui_.scaleX()),
                     lroundf(centerY * ui_.scaleY()),
                     lroundf(18 * fminf(ui_.scaleX(), ui_.scaleY())), TFT_WHITE);
}

void ControlScreen::drawDirection(int command) {
  ui_.canvas().fillRect(215 * ui_.scaleX(), 215 * ui_.scaleY(), 290 * ui_.scaleX(), 120 * ui_.scaleY(), background);
  const bool stopped = (command & 0x7f) == 0;
  const uint16_t arrowColor = stopped ? TFT_BLACK : TFT_WHITE;
  if (stopped) ui_.box(directionButton, TFT_WHITE);
  // Match the base's folded arrow: CCW points left along the upper line.
  const bool ccw = (command & 0x80) != 0;
  auto mirrored = [ccw](Point p) { if (!ccw) p.x = 720 - p.x; return p; };
  ui_.stroke(mirrored({310,250}), mirrored({380,250}), 6, arrowColor);
  ui_.stroke(mirrored({310,300}), mirrored({380,300}), 6, arrowColor);
  Point previous = {380,250};
  for (int i = 1; i <= 30; ++i) {
    const float angle = (-90 + i * 6) * PI / 180;
    Point next = {380 + 25 * cosf(angle), 275 + 25 * sinf(angle)};
    ui_.stroke(mirrored(previous), mirrored(next), 6, arrowColor);
    previous = next;
  }
  const Point a=mirrored({304,250}), b=mirrored({326,238}), c=mirrored({326,266});
  ui_.canvas().fillTriangle(a.x*ui_.scaleX(),a.y*ui_.scaleY(),b.x*ui_.scaleX(),b.y*ui_.scaleY(),c.x*ui_.scaleX(),c.y*ui_.scaleY(),arrowColor);
}

void ControlScreen::drawSpeedIndicator(int power) {
  // A completed bar uses the outline color; partial bars increase in intensity.
  for (int level = 0; level < 10; ++level) {
    const Rect bar{60, 1040 - level * 48, 210 + level * 21, 36};
    int fill = power - level * 10;
    if (fill < 0) fill = 0;
    if (fill > 10) fill = 10;
    const bool red = level >= 8;
    const uint16_t outline = red ? TFT_RED : TFT_WHITE;
    // Blend from the background toward the outline in five steps per bar.
    const int r = 16 + (255 - 16) * fill / 10;
    const int g = 16 + ((red ? 0 : 255) - 16) * fill / 10;
    const int b = 32 + ((red ? 0 : 255) - 32) * fill / 10;
    const uint16_t color = fill == 0 ? background : fill == 10 ? outline : ui_.canvas().color565(r,g,b);
    ui_.canvas().fillRect(bar.x*ui_.scaleX(),bar.y*ui_.scaleY(),bar.w*ui_.scaleX(),bar.h*ui_.scaleY(),outline);
    ui_.canvas().fillRect((bar.x+3)*ui_.scaleX(),(bar.y+3)*ui_.scaleY(),(bar.w-6)*ui_.scaleX(),(bar.h-6)*ui_.scaleY(),color);
  }
}

void ControlScreen::drawSpeedControls(int speedCommand) {
  drawSpeedIndicator(speedCommand & 0x7f);
  ui_.button(plusButton, "+", TFT_WHITE, 6, TFT_BLACK);
  ui_.button(minusButton, "-", TFT_WHITE, 6, TFT_BLACK);
  ui_.button(stopButton, "STOP", 0xD986, 5);
  drawDirection(speedCommand);
}

void ControlScreen::reset() { upperBranch = lowerBranch = false; }

void ControlScreen::draw(int speedCommand) {
  ui_.canvas().fillScreen(background);
  // Larger semicircles; junctions remain on the straight portions of both loops.
  oval(40, 90, 680, 450, 10, railColor);
  oval(90, 140, 630, 400, 10, railColor);
  if (upperBranch) crossover(90,140,10,pointColor);
  if (lowerBranch) crossover(450,400,10,pointColor);
  drawSwitch(upperSwitchY, upperBranch);
  drawSwitch(lowerSwitchY, lowerBranch);
  drawSpeedControls(speedCommand);
}

bool ControlScreen::handleTap(Point point) {
  if (BleCentral::state() != BleCentral::State::Connected) return false;
  const float x = point.x, y = point.y;
  const int command = BleCentral::speedCommand();
  const int power = command & 0x7f;
  const bool reverse = (command & 0x80) != 0;

  if (stopButton.contains(x, y)) {
    BleCentral::setSpeed(0, reverse);
  } else if (plusButton.contains(x, y)) {
    BleCentral::setSpeed(power <= maximumSpeed - speedStep ? power + speedStep : maximumSpeed, reverse);
  } else if (minusButton.contains(x, y)) {
    BleCentral::setSpeed(power >= speedStep ? power - speedStep : 0, reverse);
  } else if (directionButton.contains(x, y) && power == 0) {
    BleCentral::setSpeed(0, !reverse);
  } else if (x >= 140 && x <= 580) {
    bool* selection = nullptr;
    if (fabsf(y - upperSwitchY) <= 30) selection = &upperBranch;
    else if (fabsf(y - lowerSwitchY) <= 30) selection = &lowerBranch;
    if (selection) {
      const bool next = x < 300 ? false : x > 420 ? true : !*selection;
      if (next != *selection) {
        *selection = next;
        BleCentral::setTurnouts(lowerBranch, upperBranch);
        return true;
      }
    }
  }
  return false;
}
