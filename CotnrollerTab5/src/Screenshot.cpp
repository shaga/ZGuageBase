#include "Screenshot.h"
#include <Arduino.h>
#include <SD_MMC.h>
#include <esp_heap_caps.h>
#include <atomic>
#include <cstring>

namespace Screenshot {
namespace {
std::atomic<bool> busy{false};
uint8_t* image = nullptr;
size_t imageSize = 0;

void put16(uint8_t* p, uint16_t value) {
  p[0] = value; p[1] = value >> 8;
}
void put32(uint8_t* p, uint32_t value) {
  for (int i = 0; i < 4; ++i) p[i] = value >> (8 * i);
}

void save(void*) {
  // The Tab5 variant selects SDMMC slot 0. Slot 1 remains used by ESP-Hosted.
  // Never format the user's card when mounting fails.
  bool mounted = SD_MMC.begin("/sdcard", false, false);
  bool success = false;
  char path[48] = {};
  if (!mounted) {
    Serial.println("[Screenshot] Cannot mount microSD. Insert a formatted card and retry.");
  } else if (!SD_MMC.exists("/screenshots") && !SD_MMC.mkdir("/screenshots")) {
    Serial.println("[Screenshot] Cannot create /screenshots.");
  } else {
    for (unsigned number = 1; number <= 99999; ++number) {
      snprintf(path, sizeof(path), "/screenshots/shot-%05u.bmp", number);
      if (SD_MMC.exists(path)) continue;
      File file = SD_MMC.open(path, FILE_WRITE);
      if (file) {
        size_t written = 0;
        while (written < imageSize) {
          const size_t remaining = imageSize - written;
          const size_t chunk = remaining < 16384 ? remaining : 16384;
          const size_t count = file.write(image + written, chunk);
          if (count != chunk) break;
          written += count;
          vTaskDelay(1);
        }
        file.flush();
        success = written == imageSize && file.size() == imageSize;
        file.close();
        if (!success) SD_MMC.remove(path); // Only our newly created partial file.
      }
      break;
    }
    if (success) Serial.printf("[Screenshot] Saved %s (%u bytes)\n", path, static_cast<unsigned>(imageSize));
    else Serial.println("[Screenshot] Save failed. Check card space/write access and retry.");
  }
  if (mounted) SD_MMC.end();
  heap_caps_free(image);
  image = nullptr;
  busy.store(false);
  vTaskDelete(nullptr);
}

void capture(M5Canvas& canvas) {
  if (busy.exchange(true)) {
    Serial.println("[Screenshot] Already saving; wait for completion.");
    return;
  }
  const int w = canvas.width(), h = canvas.height();
  const size_t stride = (w * 3 + 3) & ~size_t(3);
  imageSize = 54 + stride * h;
  image = static_cast<uint8_t*>(heap_caps_calloc(1, imageSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!image) {
    Serial.println("[Screenshot] Not enough PSRAM for capture.");
    busy.store(false);
    return;
  }
  image[0] = 'B'; image[1] = 'M';
  put32(image + 2, imageSize);
  put32(image + 10, 54);
  put32(image + 14, 40);
  put32(image + 18, w);
  put32(image + 22, static_cast<uint32_t>(-h)); // Top-down bitmap.
  put16(image + 26, 1);
  put16(image + 28, 24);
  put32(image + 34, stride * h);
  // Snapshot on the UI task: the writer never reads the live canvas.
  for (int y = 0; y < h; ++y) {
    uint8_t* row = image + 54 + y * stride;
    canvas.readRectRGB(0, y, w, 1, row); // Library returns R,G,B bytes.
    for (int x = 0; x < w; ++x) {
      uint8_t r = row[x * 3];
      row[x * 3] = row[x * 3 + 2];
      row[x * 3 + 2] = r; // BMP uses B,G,R bytes.
    }
  }
  Serial.println("[Screenshot] Captured; saving to microSD...");
  if (xTaskCreate(save, "screenshot", 4096, nullptr, 1, nullptr) != pdPASS) {
    heap_caps_free(image);
    image = nullptr;
    busy.store(false);
    Serial.println("[Screenshot] Could not start save task.");
  }
}
}

void poll(M5Canvas& canvas) {
  static char command[32];
  static size_t length = 0;
  static bool overflow = false;
  // Bound work per frame even when serial input is continuous.
  for (int n = 0; n < 64 && Serial.available(); ++n) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r' || c == '\n') {
      command[length] = 0;
      if (overflow) Serial.println("[Screenshot] Command too long.");
      else if (length && strcmp(command, "screenshot") == 0) capture(canvas);
      else if (length) Serial.println("[Screenshot] Command: screenshot (then Enter)");
      length = 0;
      overflow = false;
    } else if (length < sizeof(command) - 1) command[length++] = c;
    else overflow = true;
  }
}
}
