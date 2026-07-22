#pragma once

#include <Arduino.h>

class Whack1DGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  enum class Phase : uint8_t {
    Playing,
    Error,
    GameOver,
  };

  void spawnWave(uint32_t now);
  void loseLife(uint32_t now);
  uint8_t targetCount() const;
  uint16_t waveDuration() const;
  uint16_t nextRandom();
  static int8_t pressedZone();
  static uint32_t zoneColor(uint8_t zone, bool bright);

  Phase phase_ = Phase::Playing;
  uint32_t waveStartedAt_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  uint8_t activeMask_ = 0;
  uint8_t lives_ = 0;
};
