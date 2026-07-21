#pragma once

#include <Arduino.h>

class Snake1DGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;

 private:
  enum class Phase : uint8_t {
    Playing,
    GameOver,
  };

  struct Segment {
    uint8_t colorIndex;
    uint8_t hitPoints;
    bool special;
  };

  struct Shot {
    uint16_t position;
    uint32_t lastStepAt;
    uint8_t colorIndex;
    bool active;
  };

  struct Blast {
    uint16_t position;
    uint32_t startedAt;
    uint32_t color;
    uint8_t intensity;
    bool active;
  };

  void resetObjects();
  void createInitialSnake();
  bool appendSegment(bool special);
  void appendAtTail();
  void removeFrontSegment();
  void trimTailToStrip();
  uint16_t totalWidthPixels() const;
  uint8_t segmentWidthCells(const Segment& segment) const;

  void handleButtonPress(uint32_t now);
  void launchShot(uint8_t colorIndex, uint32_t now);
  void updateShots(uint32_t now);
  bool resolveShotCollision(Shot& shot, uint32_t now);
  void updateSnake(uint32_t now);
  void handleBreach(uint32_t now);
  uint16_t effectiveStepInterval() const;

  void startBlast(uint16_t position, uint32_t color, uint8_t intensity,
                  uint32_t now);
  void updateBlasts(uint32_t now);
  uint8_t nextColorIndex();
  uint8_t nextRequiredColor(uint8_t previous);

  static int8_t pressedColorIndex();
  static uint32_t colorForIndex(uint8_t index);
  static uint32_t rainbowColor(uint8_t index);
  static uint32_t scaleColor(uint32_t color, uint8_t scale);

  Segment segments_[48] = {};
  Shot shots_[4] = {};
  Blast blasts_[4] = {};
  Phase phase_ = Phase::Playing;
  uint16_t headPosition_ = 0;
  uint16_t baseStepIntervalMs_ = 0;
  uint16_t randomState_ = 1;
  uint32_t lastSnakeStepAt_ = 0;
  uint16_t score_ = 0;
  uint8_t combo_ = 0;
  uint8_t lives_ = 0;
  uint8_t segmentCount_ = 0;
};
