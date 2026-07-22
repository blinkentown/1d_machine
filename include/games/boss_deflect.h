#pragma once

#include <Arduino.h>

class BossDeflectGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  enum class Phase : uint8_t {
    Incoming,
    Reflected,
    Feedback,
    LevelClear,
    GameOver,
  };

  void startLevel(uint32_t now);
  void spawnAttack(uint32_t now);
  void moveIncoming(uint32_t now);
  void moveReflected(uint32_t now);
  void failDefense(uint32_t now);
  void hitBoss(uint32_t now);
  bool attackInGate() const;
  uint8_t healthForLevel() const;
  uint16_t incomingStepInterval() const;
  uint16_t nextRandom();
  static int8_t pressedColor();
  static uint32_t colorFor(uint8_t color);

  Phase phase_ = Phase::Incoming;
  uint32_t lastStepAt_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  uint16_t attackPosition_ = 0;
  uint8_t attackColor_ = 0;
  uint8_t bossHealth_ = 0;
  uint8_t lives_ = 0;
  uint8_t level_ = 1;
  bool feedbackSuccess_ = false;
};
