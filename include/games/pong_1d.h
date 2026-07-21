#pragma once

#include <Arduino.h>

class Pong1DGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;

 private:
  enum class Phase : uint8_t {
    Playing,
    PointDelay,
    GameOver,
  };

  void serve(uint32_t now);
  void handleHits(uint32_t now);
  void applyHit(uint8_t depth, int8_t playerSide, uint32_t now);
  void moveBall(uint32_t now);
  void awardPoint(bool leftPlayerScored, uint32_t now);
  void printScore() const;
  void renderPerfectHit(uint32_t now) const;

  Phase phase_ = Phase::Playing;
  uint16_t ballPosition_ = 0;
  int8_t ballDirection_ = 1;
  int8_t nextServeDirection_ = 1;
  uint16_t serveIntervalMs_ = 0;
  uint16_t stepIntervalMs_ = 0;
  uint32_t lastStepAt_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint32_t perfectHitAt_ = 0;
  int8_t perfectHitSide_ = 0;
  uint8_t leftScore_ = 0;
  uint8_t rightScore_ = 0;
};
