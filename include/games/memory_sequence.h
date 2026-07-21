#pragma once

#include <Arduino.h>

class MemorySequenceGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;

 private:
  enum class Phase : uint8_t {
    Showing,
    PlayerInput,
    RoundSuccess,
    GameOver,
  };

  void beginShowing(uint32_t now);
  void fail(uint32_t now);
  uint8_t sequenceColor(uint8_t index) const;
  static uint16_t advanceRandom(uint16_t state);
  static int8_t pressedColor();
  static uint32_t colorFor(uint8_t color);
  static uint8_t stationCell(uint8_t color);

  Phase phase_ = Phase::Showing;
  uint32_t phaseChangedAt_ = 0;
  uint32_t lastInputAt_ = 0;
  uint16_t sequenceSeed_ = 1;
  uint8_t length_ = 1;
  uint8_t inputIndex_ = 0;
  uint8_t lastColor_ = 0;
  bool won_ = false;
};
