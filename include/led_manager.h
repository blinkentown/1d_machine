#pragma once

#include <Arduino.h>

namespace LedManager {

void begin();
void setPowerLimits(uint8_t brightness, uint16_t maxMilliamps);
void clearStrip();
void setStripPixel(uint16_t index, uint32_t color);
void setStripRange(uint16_t start, uint16_t count, uint32_t color);
void setGameCell(uint8_t cell, uint32_t color);
void setModePixel(uint32_t color);
void show();

}  // namespace LedManager
