#pragma once

#include <Arduino.h>

class Catch1DGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  enum class Phase : uint8_t {
    Playing,
    HitEffect,
    GameOver,
  };

  bool markerInTarget() const;
  uint8_t targetWidth() const;
  uint16_t targetStart() const;
  void moveMarker(uint32_t now);
  static bool anyColorPressed();

  Phase phase_ = Phase::Playing;
  uint32_t lastStepAt_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint16_t score_ = 0;
  uint16_t markerPosition_ = 0;
  uint16_t stepIntervalMs_ = 0;
  int8_t direction_ = 1;
};
