#pragma once
#include "UiDrawing.h"
#include "BleCentral.h"

class DeviceSelectScreen {
 public:
  explicit DeviceSelectScreen(UiDrawing& drawing) : ui_(drawing) {}
  bool refreshDevices();
  void draw(BleCentral::State state, int selected);
  // True when a new device was selected, so local controls can be reset.
  bool handleTap(Ui::Point point);
 private:
  UiDrawing& ui_;
  BleCentral::Device devices_[8]{};
  int count_ = 0;
};
