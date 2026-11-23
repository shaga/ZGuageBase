#pragma once

enum class Cursor { Speed, NearTurnout, FarTurnout };

void drawNearTurnoutDirection();
void drawFarTurnoutDirection();
void drawDirectionMark(bool clear = false);
void drawPower();
void drawCursor();
