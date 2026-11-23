#pragma once
namespace AppUi {
// Allocates the PSRAM canvas and presents the initial selection screen.
bool begin();
// Reads touch, dispatches taps, updates the canvas, then handles screenshots.
void update();
}
