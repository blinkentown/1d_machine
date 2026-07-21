#pragma once

#include <Arduino.h>

class ColourShooterGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;

 private:
  enum class Phase : uint8_t {
    Playing,
    Feedback,
    GameOver,
  };

  void spawnTarget(uint32_t now);
  void handleButtonPress(uint32_t now);
  void handleMiss(uint32_t now);
  void loseLife(uint32_t now, const __FlashStringHelper* reason);
  void beginFeedback(uint32_t now, uint16_t durationMs, uint32_t color);
  uint8_t nextColorIndex();
  static uint32_t colorForIndex(uint8_t index);
  static int8_t pressedColorIndex();

  Phase phase_ = Phase::Playing;
  uint16_t targetPosition_ = 0;
  uint16_t stepIntervalMs_ = 0;
  uint16_t feedbackDurationMs_ = 0;
  uint16_t randomState_ = 1;
  uint32_t lastStepAt_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint32_t feedbackColor_ = 0;
  uint16_t score_ = 0;
  uint8_t lives_ = 0;
  uint8_t targetColorIndex_ = 0;
};
