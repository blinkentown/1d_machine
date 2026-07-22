#pragma once

#include <Arduino.h>

class MeteorDodgeGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }
  uint8_t lives() const { return lives_; }

 private:
  enum class Phase : uint8_t {
    Warning,
    Impact,
    GameOver,
  };

  void startMeteor(uint32_t now);
  void resolveImpact(uint32_t now);
  void move(int8_t direction);
  uint16_t nextRandom();

  Phase phase_ = Phase::Warning;
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  uint8_t playerCell_ = 1;
  uint8_t meteorCell_ = 1;
  uint8_t lives_ = 0;
  uint8_t shields_ = 0;
  bool shieldActive_ = false;
};
