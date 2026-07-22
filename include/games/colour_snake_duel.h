#pragma once

#include <Arduino.h>

#include "config.h"

class ColourSnakeDuelGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint8_t player1Score() const { return player1Score_; }
  uint8_t player2Score() const { return player2Score_; }

 private:
  static constexpr uint8_t CELL_COUNT =
      Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;
  static constexpr uint8_t HALF_CELLS = CELL_COUNT / 2U;

  enum class Phase : uint8_t {
    Playing,
    RoundResult,
    GameOver,
  };

  struct Shot {
    uint32_t lastStepAt;
    uint16_t position;
    uint8_t colorIndex;
    bool active;
  };

  struct HitEffect {
    uint32_t startedAt;
    uint16_t position;
    uint8_t colorIndex;
    bool active;
  };

  void startRound(uint32_t now);
  void launchShot(bool player1, uint8_t colorIndex, uint32_t now);
  void updateShot(Shot& shot, bool player1, uint32_t now);
  bool growSidePixel(bool player1);
  void removeHead(bool player1);
  void finishRound(bool player1Breached, bool player2Breached,
                   uint32_t now);
  void startHitEffect(bool player1, uint16_t position, uint8_t colorIndex,
                      uint32_t now);
  void updateEffects(uint32_t now);
  uint16_t growthStepInterval(uint32_t now) const;
  uint16_t nextRandom();
  static uint32_t colorFor(bool player1, uint8_t colorIndex);
  static uint32_t scaleColor(uint32_t color, uint8_t scale);
  static bool anyPlayerButtonPressed();

  Phase phase_ = Phase::Playing;
  uint8_t player1Colors_[HALF_CELLS] = {};
  uint8_t player2Colors_[HALF_CELLS] = {};
  Shot player1Shot_ = {};
  Shot player2Shot_ = {};
  HitEffect player1Effect_ = {};
  HitEffect player2Effect_ = {};
  uint32_t roundStartedAt_ = 0;
  uint32_t lastGrowthAt_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint16_t randomState_ = 1;
  uint32_t lastPenaltyStepAt_ = 0;
  uint8_t player1LengthPixels_ = 0;
  uint8_t player2LengthPixels_ = 0;
  uint8_t player1PenaltyPixels_ = 0;
  uint8_t player2PenaltyPixels_ = 0;
  uint8_t player1Score_ = 0;
  uint8_t player2Score_ = 0;
  int8_t roundWinner_ = 0;
};
