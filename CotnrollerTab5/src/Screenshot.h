#pragma once
#include <M5GFX.h>

namespace Screenshot {
// Call after presenting a completed frame. Accepts screenshot + CR/LF.
void poll(M5Canvas& canvas);
}
