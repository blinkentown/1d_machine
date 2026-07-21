#pragma once

#include <Arduino.h>

class TwangGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;

 private:
  enum class Phase : uint8_t {
    Playing,
    LevelClear,
    GameOver,
  };

  void startLevel(uint32_t now);
  void generateDungeon();
  void move(int8_t direction, bool dash, uint32_t now);
  void attack(uint32_t now);
  void loseLife(uint32_t now, const __FlashStringHelper* reason);
  uint16_t nextRandom();
  static uint32_t cellBit(uint8_t cell);

  Phase phase_ = Phase::Playing;
  uint32_t enemyMask_ = 0;
  uint32_t lavaMask_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint32_t effectStartedAt_ = 0;
  uint32_t lastDashAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  int8_t facing_ = 1;
  int8_t effectDirection_ = 1;
  uint8_t playerCell_ = 1;
  uint8_t effectCell_ = 1;
  uint8_t lives_ = 0;
  uint8_t level_ = 1;
  bool effectActive_ = false;
};
