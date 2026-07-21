#pragma once

#include <Arduino.h>

namespace InputManager {

enum class Button : uint8_t {
  Game1,
  Game2,
  Game3,
  Game4,
  EncoderClick,
  Setup,
  ModeSelect,
};

void begin();
void update(uint32_t now);

bool isHeld(Button button);
bool wasPressed(Button button);
bool wasReleased(Button button);

int8_t takeEncoderDelta();

}  // namespace InputManager
