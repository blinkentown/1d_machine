#pragma once

#include <Arduino.h>

class PowerStressTest {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void stop();
  void render() const;
  bool isFinished() const;

 private:
  uint32_t startedAt_ = 0;
  bool active_ = false;
};
