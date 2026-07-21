#include "games/reaction_race.h"

#include "config.h"
#include "controls.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t CELL_COUNT =
    Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;
constexpr uint8_t RACE_STEPS = CELL_COUNT / 2U - 1U;

}  // namespace

void ReactionRaceGame::start(uint32_t now) {
  player1Score_ = 0;
  player2Score_ = 0;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  Serial.println(F("Reaction Race started"));
  Serial.println(F("P1 alternates Red/Green; P2 alternates Blue/Yellow"));
  prepareRound(now);
}

uint16_t ReactionRaceGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

void ReactionRaceGame::prepareRound(uint32_t now) {
  player1Progress_ = 0;
  player2Progress_ = 0;
  player1SecondaryNext_ = false;
  player2SecondaryNext_ = false;
  roundWinner_ = 0;
  phaseChangedAt_ = now;
  goAt_ = now + Config::REACTION_WAIT_MIN_MS +
          nextRandom() %
              (Config::REACTION_WAIT_MAX_MS - Config::REACTION_WAIT_MIN_MS);
  phase_ = Phase::Waiting;
  Serial.println(F("Wait for green center signal"));
}

void ReactionRaceGame::finishRound(int8_t winner, uint32_t now,
                                    bool falseStart) {
  roundWinner_ = winner;
  phaseChangedAt_ = now;

  if (winner < 0) {
    ++player1Score_;
  } else if (winner > 0) {
    ++player2Score_;
  }

  Serial.print(falseStart ? F("False start. Score ") : F("Round. Score "));
  Serial.print(player1Score_);
  Serial.print(F(" - "));
  Serial.println(player2Score_);

  phase_ = player1Score_ >= Config::REACTION_ROUNDS_TO_WIN ||
                   player2Score_ >= Config::REACTION_ROUNDS_TO_WIN
               ? Phase::GameOver
               : Phase::RoundResult;
  if (phase_ == Phase::GameOver) {
    Serial.println(F("Reaction match over. Press any color to restart"));
  }
}

void ReactionRaceGame::handleRaceInputs(uint32_t now) {
  const bool player1Correct =
      player1SecondaryNext_
          ? InputManager::wasPressed(Controls::PLAYER_1_SECONDARY)
          : InputManager::wasPressed(Controls::PLAYER_1_PRIMARY);
  const bool player2Correct =
      player2SecondaryNext_
          ? InputManager::wasPressed(Controls::PLAYER_2_SECONDARY)
          : InputManager::wasPressed(Controls::PLAYER_2_PRIMARY);

  if (player1Correct) {
    ++player1Progress_;
    player1SecondaryNext_ = !player1SecondaryNext_;
  }
  if (player2Correct) {
    ++player2Progress_;
    player2SecondaryNext_ = !player2SecondaryNext_;
  }

  const bool player1Finished = player1Progress_ >= RACE_STEPS;
  const bool player2Finished = player2Progress_ >= RACE_STEPS;
  if (player1Finished || player2Finished) {
    finishRound(player1Finished == player2Finished
                    ? 0
                    : (player1Finished ? -1 : 1),
                now, false);
  }
}

void ReactionRaceGame::update(uint32_t now) {
  const bool player1Pressed =
      InputManager::wasPressed(Controls::PLAYER_1_PRIMARY) ||
      InputManager::wasPressed(Controls::PLAYER_1_SECONDARY);
  const bool player2Pressed =
      InputManager::wasPressed(Controls::PLAYER_2_PRIMARY) ||
      InputManager::wasPressed(Controls::PLAYER_2_SECONDARY);

  if (phase_ == Phase::GameOver) {
    if (player1Pressed || player2Pressed) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::RoundResult) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::REACTION_ROUND_RESULT_MS) {
      prepareRound(now);
    }
    return;
  }

  if (phase_ == Phase::Waiting) {
    if (player1Pressed || player2Pressed) {
      finishRound(player1Pressed == player2Pressed
                      ? 0
                      : (player1Pressed ? 1 : -1),
                  now, true);
      return;
    }
    if (static_cast<int32_t>(now - goAt_) >= 0) {
      phase_ = Phase::Racing;
      phaseChangedAt_ = now;
      Serial.println(F("GO"));
    }
    return;
  }

  handleRaceInputs(now);
}

void ReactionRaceGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::GameOver) {
    const uint32_t winnerColor =
        player1Score_ > player2Score_ ? Config::REACTION_PLAYER_1_COLOR
                                      : Config::REACTION_PLAYER_2_COLOR;
    if ((now / 140U) % 2U == 0U) {
      for (uint8_t cell = 0; cell < CELL_COUNT; ++cell) {
        LedManager::setGameCell(cell,
                                (cell & 1U) == 0U ? winnerColor : 0xFFFFFFUL);
      }
    }
    return;
  }

  if (phase_ == Phase::RoundResult) {
    if ((now / 120U) % 2U == 0U) {
      if (roundWinner_ <= 0) {
        LedManager::setStripRange(0, Config::LED_COUNT / 2U,
                                  roundWinner_ < 0
                                      ? Config::REACTION_PLAYER_1_COLOR
                                      : Config::REACTION_FALSE_START_COLOR);
      }
      if (roundWinner_ >= 0) {
        LedManager::setStripRange(Config::LED_COUNT / 2U,
                                  Config::LED_COUNT / 2U,
                                  roundWinner_ > 0
                                      ? Config::REACTION_PLAYER_2_COLOR
                                      : Config::REACTION_FALSE_START_COLOR);
      }
    }
    return;
  }

  if (phase_ == Phase::Waiting) {
    LedManager::setGameCell(0, 0x300000UL);
    LedManager::setGameCell(CELL_COUNT - 1U, 0x000030UL);
    const uint32_t waitColor =
        ((now / 180U) & 1U) == 0U ? Config::REACTION_WAIT_COLOR : 0x301800UL;
    LedManager::setGameCell(CELL_COUNT / 2U - 1U, waitColor);
    LedManager::setGameCell(CELL_COUNT / 2U, waitColor);
    return;
  }

  LedManager::setGameCell(CELL_COUNT / 2U - 1U, Config::REACTION_GO_COLOR);
  LedManager::setGameCell(CELL_COUNT / 2U, Config::REACTION_GO_COLOR);
  for (uint8_t cell = 0; cell <= player1Progress_; ++cell) {
    LedManager::setGameCell(cell, 0x300000UL);
  }
  for (uint8_t cell = 0; cell <= player2Progress_; ++cell) {
    LedManager::setGameCell(CELL_COUNT - 1U - cell, 0x000030UL);
  }

  LedManager::setGameCell(
      player1Progress_,
      player1SecondaryNext_ ? Config::BUTTON_2_COLOR : Config::BUTTON_1_COLOR);
  LedManager::setGameCell(
      CELL_COUNT - 1U - player2Progress_,
      player2SecondaryNext_ ? Config::BUTTON_4_COLOR : Config::BUTTON_3_COLOR);
}
