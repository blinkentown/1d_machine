#pragma once

#include <Arduino.h>

#include "games/boss_deflect.h"
#include "games/colour_gate.h"

class ColourQuestGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const;

 private:
  enum class Stage : uint8_t {
    Gate,
    Transition,
    Boss,
  };

  Stage stage_ = Stage::Gate;
  ColourGateGame gate_;
  BossDeflectGame boss_;
  uint32_t transitionStartedAt_ = 0;
};
