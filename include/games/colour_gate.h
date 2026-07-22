#pragma once

#include <Arduino.h>

class ColourGateGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  enum class Phase : uint8_t {
    Playing,
    Feedback,
    GameOver,
  };

  void spawnCue(uint32_t now);
  void moveCue(uint32_t now);
  void resolveCue(bool success, uint32_t now);
  bool cueInGate() const;
  uint16_t nextRandom();
  static int8_t pressedColor();
  static bool anyColorPressed();
  static uint32_t colorFor(uint8_t color);

  Phase phase_ = Phase::Playing;
  uint32_t lastStepAt_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  uint16_t cuePosition_ = 0;
  uint16_t stepIntervalMs_ = 0;
  uint8_t cueColor_ = 0;
  uint8_t lives_ = 0;
  bool lastCueSucceeded_ = false;
};
