#pragma once

namespace BleCentral {
struct Device { char name[64]; char address[18]; };
int devices(Device* output, int capacity);
bool selectDevice(int index);
void returnToSelection();
int selectedDevice();
enum class State { Starting, Scanning, Connecting, Checking, Connected, Retrying, Incompatible, Failed };
void begin();
// Store the latest requested pair; sent asynchronously, including on reconnect.
void setTurnouts(bool lowerBranch, bool upperBranch);
bool setSpeed(int power, bool counterClockwise);
int speedCommand();
State state();
const char* statusText(State state);
}
