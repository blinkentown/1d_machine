#pragma once

#include <Arduino.h>

class Hanoi1DGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  enum class Phase : uint8_t {
    Playing,
    Feedback,
    LevelClear,
  };

  uint8_t diskPeg(uint8_t disk) const;
  void setDiskPeg(uint8_t disk, uint8_t peg);
  int8_t topDisk(uint8_t peg) const;
  void choosePeg(uint8_t peg, uint32_t now);
  void resetLevel(uint32_t now);
  bool levelComplete() const;
  static uint32_t diskColor(uint8_t disk);

  Phase phase_ = Phase::Playing;
  uint32_t phaseChangedAt_ = 0;
  uint16_t positions_ = 0;
  uint16_t score_ = 0;
  uint8_t diskCount_ = 3;
  int8_t selectedPeg_ = -1;
  uint8_t feedbackPeg_ = 0;
};
