#pragma once

#include <Arduino.h>

namespace PowerModeManager {

void begin(uint32_t now);
void update(uint32_t now);
void toggleMode();
bool isReady();
bool isPsuMode();

}  // namespace PowerModeManager
