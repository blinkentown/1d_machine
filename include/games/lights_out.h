#pragma once

#include <Arduino.h>

class LightsOutGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  enum class Phase : uint8_t {
    Playing,
    Solved,
  };

  void generatePuzzle(uint32_t now);
  void toggleAt(uint8_t cell);
  uint16_t nextRandom();

  Phase phase_ = Phase::Playing;
  uint32_t lights_ = 0;
  uint32_t initialLights_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  uint8_t level_ = 1;
  uint8_t cursor_ = 0;
};
