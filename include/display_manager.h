#pragma once

#include <Arduino.h>

namespace DisplayManager {

enum class Mode : uint8_t {
  Twang,
  ColourShooter,
  Pong1D,
  ReactionRace,
  ColourSnakeDuel,
  Count,
};

void begin();
void clear();
void showSelection(Mode mode);
void showSingleScore(uint16_t player1Score);
void showVersusScore(uint16_t player1Score, uint16_t player2Score);

}  // namespace DisplayManager
