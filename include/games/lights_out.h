#pragma once

#include <Arduino.h>

class LightsOutGame {
 public:
  void start(uint32_t now, bool versus);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return scores_[0]; }
  uint16_t player1Score() const { return scores_[0]; }
  uint16_t player2Score() const { return scores_[1]; }

 private:
  enum class Phase : uint8_t {
    Playing,
    Solved,
    RoundWon,
    GameOver,
  };

  void generatePuzzle(uint32_t now);
  uint32_t generateMask(uint8_t cellCount, uint8_t scrambleCount);
  void moveCursor(uint8_t player, int8_t delta, uint8_t cellCount);
  void toggleAt(uint8_t player, uint8_t cellCount);
  void checkDuelResult(uint32_t now);
  uint16_t nextRandom();

  Phase phase_ = Phase::Playing;
  uint32_t lights_[2] = {};
  uint32_t initialLights_[2] = {};
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t scores_[2] = {};
  uint8_t level_ = 1;
  uint8_t cursors_[2] = {};
  uint8_t winner_ = 0;
  bool versus_ = false;
};
