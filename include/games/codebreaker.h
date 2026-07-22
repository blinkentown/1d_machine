#pragma once

#include <Arduino.h>

#include "config.h"

class CodebreakerGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  enum class Phase : uint8_t {
    Input,
    Feedback,
    RoundSuccess,
    GameOver,
  };

  void startRound(uint32_t now);
  void evaluateGuess(uint32_t now);
  uint8_t secretColor(uint8_t index) const;
  uint16_t nextRandom();
  static int8_t pressedColor();
  static uint32_t colorFor(uint8_t color);

  uint8_t guess_[Config::CODEBREAKER_CODE_LENGTH] = {};
  Phase phase_ = Phase::Input;
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t secretBits_ = 0;
  uint16_t score_ = 0;
  uint8_t inputCount_ = 0;
  uint8_t attemptsLeft_ = 0;
  uint8_t exactMatches_ = 0;
  uint8_t misplacedMatches_ = 0;
};
