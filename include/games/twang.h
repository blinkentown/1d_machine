#pragma once

#include <Arduino.h>

class TwangGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint8_t level() const { return level_; }
  uint16_t score() const { return score_; }
  uint8_t lives() const { return lives_; }

 private:
  enum class Phase : uint8_t {
    Playing,
    LevelClear,
    GameOver,
  };

  void startLevel(uint32_t now);
  void generateDungeon();
  bool move(int8_t direction, uint32_t now);
  void jump(uint32_t now);
  void attack(uint32_t now);
  void updatePlayerAnimation(uint32_t now);
  void loseLife(uint32_t now);
  uint16_t nextRandom();
  static uint32_t cellBit(uint8_t cell);

  Phase phase_ = Phase::Playing;
  uint32_t enemyMask_ = 0;
  uint32_t lavaMask_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint32_t effectStartedAt_ = 0;
  uint32_t lastPlayerPixelStepAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  uint16_t playerRenderPosition_ = 0;
  int8_t facing_ = 1;
  uint8_t playerCell_ = 1;
  uint8_t effectCell_ = 1;
  uint8_t lives_ = 0;
  uint8_t level_ = 1;
  bool effectActive_ = false;
  bool exitPending_ = false;
};
