#include "UiDrawing.h"
#include <math.h>
using namespace Ui;

void UiDrawing::stroke(Point a, Point b, float width, uint16_t color) {
  const float radius = width * fminf(scaleX(), scaleY()) / 2;
  canvas_.drawWideLine(a.x * scaleX(), a.y * scaleY(),
                       b.x * scaleX(), b.y * scaleY(), radius * 2, color);
  canvas_.fillCircle(lroundf(b.x * scaleX()), lroundf(b.y * scaleY()),
                     lroundf(radius), color);
}

void UiDrawing::label(const char* text, int x, int y, uint16_t color, int size) {
  canvas_.setTextColor(color, background);
  canvas_.setTextSize(size);
  canvas_.setCursor(lroundf(x * scaleX()), lroundf(y * scaleY()));
  canvas_.print(text);
}

void UiDrawing::box(Rect r, uint16_t color) {
  canvas_.fillRoundRect(lroundf(r.x * scaleX()), lroundf(r.y * scaleY()),
                        lroundf(r.w * scaleX()), lroundf(r.h * scaleY()),
                        lroundf(14 * fminf(scaleX(), scaleY())), color);
}

void UiDrawing::button(Rect r, const char* text, uint16_t color, int size, uint16_t textColor) {
  box(r, color);
  canvas_.setTextColor(textColor, color);
  canvas_.setTextSize(size);
  canvas_.drawCenterString(text, lroundf((r.x + r.w / 2) * scaleX()),
                          lroundf((r.y + r.h / 2 - size * 4) * scaleY()));
}
