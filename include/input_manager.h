#pragma once

#include <Arduino.h>

namespace InputManager {

enum class Button : uint8_t {
  Red,
  Green,
  Blue,
  Yellow,
  EncoderClick,
  Setup,
  ModeSelect,
};

enum class Encoder : uint8_t {
  Player1,
  Player2,
};

void begin();
void update(uint32_t now);

bool isHeld(Button button);
bool wasPressed(Button button);
bool wasReleased(Button button);

int8_t encoderDelta(Encoder encoder);

}  // namespace InputManager
