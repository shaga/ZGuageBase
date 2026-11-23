#pragma once
#include "UiDrawing.h"

class ControlScreen {
 public:
  explicit ControlScreen(UiDrawing& drawing) : ui_(drawing) {}
  void reset();
  void draw(int speedCommand);
  void drawSpeedControls(int speedCommand);
  // True means turnout artwork changed. Speed updates are detected by AppUi.
  bool handleTap(Ui::Point point);
 private:
  UiDrawing& ui_;
  bool upperBranch = false, lowerBranch = false;
  void oval(float left, float top, float right, float bottom, float width, uint16_t color);
  void crossover(float startY, float endY, float width, uint16_t color);
  void drawSwitch(int centerY, bool branch);
  void drawDirection(int command);
  void drawSpeedIndicator(int power);
};
