#pragma once
#include <M5GFX.h>

namespace Ui {
constexpr int width = 720, height = 1280;
constexpr uint16_t background = 0x1084, railColor = 0xCE79, pointColor = 0xFDA0, accent = 0x4EBA;
struct Point { float x, y; };
struct Rect {
  int x, y, w, h;
  bool contains(float px, float py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
  }
};
}

// Drawing and hit testing share the same 720 x 1280 logical coordinates.
class UiDrawing {
 public:
  explicit UiDrawing(M5Canvas& canvas) : canvas_(canvas) {}
  M5Canvas& canvas() { return canvas_; }
  float scaleX() const { return canvas_.width() / float(Ui::width); }
  float scaleY() const { return canvas_.height() / float(Ui::height); }
  Ui::Point toLogical(int x, int y) const { return {x / scaleX(), y / scaleY()}; }
  void stroke(Ui::Point a, Ui::Point b, float width, uint16_t color);
  void label(const char* text, int x, int y, uint16_t color, int size = 2);
  void box(Ui::Rect r, uint16_t color);
  void button(Ui::Rect r, const char* text, uint16_t color, int size = 4, uint16_t textColor = TFT_WHITE);
 private:
  M5Canvas& canvas_;
};
