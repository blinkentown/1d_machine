#pragma once

#include <Arduino.h>

class ReactionRaceGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;

 private:
  enum class Phase : uint8_t {
    Waiting,
    Racing,
    RoundResult,
    GameOver,
  };

  void prepareRound(uint32_t now);
  void finishRound(int8_t winner, uint32_t now);
  void handleRaceInputs(uint32_t now);
  uint16_t nextRandom();

  Phase phase_ = Phase::Waiting;
  uint32_t phaseChangedAt_ = 0;
  uint32_t goAt_ = 0;
  uint16_t randomState_ = 1;
  uint8_t player1Progress_ = 0;
  uint8_t player2Progress_ = 0;
  uint8_t player1Score_ = 0;
  uint8_t player2Score_ = 0;
  int8_t roundWinner_ = 0;
  bool player1SecondaryNext_ = false;
  bool player2SecondaryNext_ = false;
};
