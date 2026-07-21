#pragma once

#include <Arduino.h>

class PowerTest {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render() const;
  bool isReady() const;

 private:
  enum class Stage : uint8_t {
    DarkBaseline,
    SinglePixel,
    GameLoad,
    Ready,
  };

  void advance(uint32_t now);

  Stage stage_ = Stage::DarkBaseline;
  uint32_t stageStartedAt_ = 0;
};
