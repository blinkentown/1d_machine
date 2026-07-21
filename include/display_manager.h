#pragma once

#include <Arduino.h>

namespace DisplayManager {

enum class Mode : uint8_t {
  Twang,
  ColourShooter,
  Pong1D,
  ReactionRace,
  Snake1D,
  MeteorDodge,
  MemorySequence,
  Count,
};

void begin();
void clear();
void showSelection(Mode mode);
void showSingleScore(Mode mode, uint16_t score, uint8_t indicators = 0);
void showVersusScore(Mode mode, uint8_t leftScore, uint8_t rightScore);

}  // namespace DisplayManager
