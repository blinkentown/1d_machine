#pragma once

#include <Arduino.h>

#include "config.h"

class ColourShooterGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;

 private:
  enum class Phase : uint8_t {
    Playing,
    GameOver,
  };

  struct Target {
    uint16_t position;
    uint8_t colorIndex;
    bool active;
  };

  struct Shot {
    uint16_t position;
    uint32_t lastStepAt;
    uint8_t colorIndex;
    bool active;
  };

  struct Dissolve {
    uint16_t position;
    uint32_t startedAt;
    uint32_t color;
    bool active;
  };

  void resetObjects();
  bool spawnTarget(uint16_t position);
  void spawnTargetAtFarEnd(uint32_t now);
  void handleButtonPress(uint32_t now);
  void launchShot(uint8_t colorIndex, uint32_t now);
  void updateShots(uint32_t now);
  void updateTargets(uint32_t now);
  void updateDissolves(uint32_t now);
  bool resolveShotCollision(Shot& shot, uint32_t now);
  void startDissolve(uint16_t position, uint32_t color, uint32_t now);
  void loseLife(uint32_t now, const __FlashStringHelper* reason);
  uint8_t nextColorIndex();
  bool farEndIsClear() const;
  static uint32_t colorForIndex(uint8_t index);
  static uint32_t scaleColor(uint32_t color, uint8_t scale);
  static int8_t pressedColorIndex();

  Phase phase_ = Phase::Playing;
  Target targets_[Config::COLOUR_SHOOTER_TARGET_COUNT] = {};
  Shot shots_[Config::COLOUR_SHOOTER_MAX_SHOTS] = {};
  Dissolve dissolves_[Config::COLOUR_SHOOTER_MAX_DISSOLVES] = {};
  uint16_t stepIntervalMs_ = 0;
  uint16_t randomState_ = 1;
  uint32_t lastTargetStepAt_ = 0;
  uint32_t lastSpawnAt_ = 0;
  uint16_t score_ = 0;
  uint8_t lives_ = 0;
};
