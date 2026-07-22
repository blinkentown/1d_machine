#pragma once

#include <Arduino.h>

class Minefield1DGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  enum class Phase : uint8_t {
    Playing,
    Solved,
    GameOver,
  };

  void generateBoard(uint32_t now);
  void moveCursor(int8_t delta);
  void reveal(uint8_t cell, uint32_t now);
  void revealSafeRun(uint8_t cell);
  void relocateFirstMine(uint8_t cell);
  uint8_t adjacentMines(uint8_t cell) const;
  uint8_t adjacentFlags(uint8_t cell) const;
  void chordReveal(uint8_t cell, uint32_t now);
  void checkSolved(uint32_t now);
  uint8_t mineCountForScore() const;
  uint16_t nextRandom();

  Phase phase_ = Phase::Playing;
  uint32_t mines_ = 0;
  uint32_t revealed_ = 0;
  uint32_t flagged_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  uint8_t cursor_ = 0;
  bool firstReveal_ = true;
};
