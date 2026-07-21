#pragma once

#include <Arduino.h>

namespace LedManager {

void begin();
void setPowerLimits(uint8_t brightness, uint16_t maxMilliamps);
void clearStrip();
void setStripPixel(uint16_t index, uint32_t color);
void setModePixel(uint32_t color);
void show();

}  // namespace LedManager
