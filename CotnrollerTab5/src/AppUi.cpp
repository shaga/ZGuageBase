#include "AppUi.h"
#include "ControlScreen.h"
#include "DeviceSelectScreen.h"
#include "Screenshot.h"
#include <M5Unified.h>
#include <esp_heap_caps.h>

namespace AppUi {
namespace {
auto& display = M5.Display;
M5Canvas canvas(&display);
UiDrawing drawing(canvas);
ControlScreen controls(drawing);
DeviceSelectScreen selection(drawing);
bool canvasReady = false;
bool touchWasDown = false;
bool operating = false;
BleCentral::State shownState = BleCentral::State::Starting;
int shownSelection = -1;
int shownSpeed = -1;

void presentFrame() {
  const uint32_t started = micros();
  canvas.pushSprite(0, 0);
  display.waitDMA();
  Serial.printf("[UI] frame transfer: %lu us\n", static_cast<unsigned long>(micros() - started));
}

void logMemory(const char* stage) {
  Serial.printf("[UI] %s: internal free=%u largest=%u; PSRAM free=%u largest=%u\n",
                stage,
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)),
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)),
                static_cast<unsigned>(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM)));
}
} // namespace

bool begin() {
  display.setRotation(0);
  if (display.width()>display.height()) display.setRotation(1);
  logMemory("before sprite");
  canvas.setPsram(true);
  canvas.setColorDepth(16);
  canvasReady = psramFound() && canvas.createSprite(display.width(), display.height()) != nullptr;
  if (!canvasReady) {
    Serial.println("[UI] PSRAM sprite allocation failed; UI not started");
    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setTextSize(2);
    display.setCursor(20, 40);
    display.println("UI buffer allocation failed.");
    display.println("Check PSRAM and restart.");
    return false;
  }
  logMemory("after sprite");
  selection.refreshDevices();
  selection.draw(shownState, shownSelection);
  presentFrame();
  Serial.println("[Screenshot] Type screenshot then Enter to save to microSD.");
  return true;
}

void update() {
  if (!canvasReady) return;
  const bool devicesChanged = selection.refreshDevices();
  bool controlsChanged = false;
  int32_t x = 0, y = 0;
  const bool down = display.getTouch(&x, &y) > 0;
  if (down && !touchWasDown) {
    const Ui::Point point = drawing.toLogical(x, y);
    if (operating) controlsChanged = controls.handleTap(point);
    else if (selection.handleTap(point)) controls.reset();
  }
  touchWasDown = down;

  const auto state = BleCentral::state();
  const int selected = BleCentral::selectedDevice();
  const int speed = BleCentral::speedCommand();
  const bool connected = state == BleCentral::State::Connected && selected >= 0;
  const bool layoutChanged = controlsChanged || devicesChanged ||
      state != shownState || selected != shownSelection || connected != operating;
  if (state != shownState) logMemory(BleCentral::statusText(state));
  if (layoutChanged) {
    if (connected) controls.draw(speed);
    else selection.draw(state, selected);
  } else if (connected && speed != shownSpeed) {
    controls.drawSpeedControls(speed);
  }
  if (layoutChanged || (connected && speed != shownSpeed)) presentFrame();
  operating = connected;
  shownState = state;
  shownSelection = selected;
  shownSpeed = speed;
  Screenshot::poll(canvas);
}
} // namespace AppUi
