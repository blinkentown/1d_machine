#pragma once

#include <Arduino.h>

class NimDuelGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t player1Score() const { return scores_[0]; }
  uint16_t player2Score() const { return scores_[1]; }

 private:
  enum class Phase : uint8_t {
    Playing,
    RoundWon,
  };

  void startRound(uint32_t now);
  void adjustTake(int8_t delta);
  void confirmTake(uint32_t now);

  Phase phase_ = Phase::Playing;
  uint32_t phaseChangedAt_ = 0;
  uint16_t scores_[2] = {};
  uint8_t remaining_ = 0;
  uint8_t selectedTake_ = 1;
  uint8_t activePlayer_ = 0;
  uint8_t roundStarter_ = 0;
  uint8_t winner_ = 0;
};
