#include "DeviceSelectScreen.h"
#include <cstring>
using namespace Ui;
namespace {
constexpr Rect backButton{60, 1100, 600, 90};
constexpr Rect deviceRow(int index) { return {60, 210 + index * 100, 600, 80}; }
}

bool DeviceSelectScreen::refreshDevices() {
  BleCentral::Device next[8]{};
  const int count = BleCentral::devices(next, 8);
  const bool changed = count != count_ || memcmp(next, devices_, sizeof(next)) != 0;
  memcpy(devices_, next, sizeof(next));
  count_ = count;
  return changed;
}

void DeviceSelectScreen::draw(BleCentral::State state, int selected) {
  ui_.canvas().fillScreen(background);
  ui_.label("Select device", 60, 70, TFT_WHITE, 4);
  if (selected >= 0) {
    ui_.label(selected < count_ ? devices_[selected].name : "ZGuageBase", 60, 210, accent, 3);
    ui_.label(state == BleCentral::State::Incompatible ? "Incompatible device" :
          state == BleCentral::State::Connecting || state == BleCentral::State::Checking ?
          "Connecting..." : "Searching / reconnecting...", 60, 290, railColor);
    ui_.button(backButton, "Select another device", 0x2945, 2);
  } else {
    ui_.label(state == BleCentral::State::Failed ? "BLE init failed - restart device" :
          "Scanning...", 60, 140, railColor);
    for (int i = 0; i < count_; ++i)
      ui_.button(deviceRow(i), devices_[i].name, 0x2945, 2);
    if (!count_) ui_.label("Waiting for ZGuageBase...", 60, 250, railColor);
  }
}

bool DeviceSelectScreen::handleTap(Point point) {
  if (BleCentral::selectedDevice() >= 0) {
    if (backButton.contains(point.x, point.y)) BleCentral::returnToSelection();
  } else {
    for (int i = 0; i < count_; ++i) {
      if (deviceRow(i).contains(point.x, point.y) && BleCentral::selectDevice(i)) return true;
    }
  }
  return false;
}
