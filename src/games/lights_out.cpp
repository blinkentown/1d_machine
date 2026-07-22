#include "games/lights_out.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t CELL_COUNT =
    Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;
constexpr uint8_t DUEL_CELL_COUNT = CELL_COUNT / 2U;

static_assert(Config::LED_COUNT % Config::GAME_PIXEL_WIDTH == 0,
              "Lights Out requires complete logical cells");
static_assert(CELL_COUNT <= 32, "Lights Out uses a 32-bit cell mask");
static_assert(CELL_COUNT % 2U == 0, "Lights Out duel needs equal boards");

uint32_t toggledMask(uint32_t mask, uint8_t cell, uint8_t cellCount) {
  mask ^= 1UL << cell;
  if (cell > 0) {
    mask ^= 1UL << (cell - 1U);
  }
  if (cell + 1U < cellCount) {
    mask ^= 1UL << (cell + 1U);
  }
  return mask;
}

}  // namespace

void LightsOutGame::start(uint32_t now, bool versus) {
  versus_ = versus;
  scores_[0] = 0;
  scores_[1] = 0;
  level_ = 1;
  winner_ = 0;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(versus_ ? F("Lights Out duel started")
                        : F("Lights Out started"));
  generatePuzzle(now);
}

uint16_t LightsOutGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

uint32_t LightsOutGame::generateMask(uint8_t cellCount,
                                     uint8_t scrambleCount) {
  uint32_t mask = 0;
  uint8_t previous = cellCount;
  for (uint8_t index = 0; index < scrambleCount; ++index) {
    uint8_t cell = static_cast<uint8_t>(nextRandom() % cellCount);
    if (cell == previous) {
      cell = static_cast<uint8_t>((cell + 1U) % cellCount);
    }
    mask = toggledMask(mask, cell, cellCount);
    previous = cell;
  }
  if (mask == 0) {
    mask = toggledMask(mask, static_cast<uint8_t>(nextRandom() % cellCount),
                       cellCount);
  }
  return mask;
}

void LightsOutGame::generatePuzzle(uint32_t now) {
  const uint8_t extra =
      level_ < Config::LIGHTS_OUT_MAX_EXTRA_SCRAMBLES
          ? level_
          : Config::LIGHTS_OUT_MAX_EXTRA_SCRAMBLES;
  const uint8_t scrambleCount =
      Config::LIGHTS_OUT_BASE_SCRAMBLES + extra;
  const uint8_t cellCount = versus_ ? DUEL_CELL_COUNT : CELL_COUNT;
  const uint32_t mask = generateMask(cellCount, scrambleCount);

  lights_[0] = mask;
  initialLights_[0] = mask;
  lights_[1] = versus_ ? mask : 0;
  initialLights_[1] = lights_[1];

  for (uint8_t player = 0; player < 2U; ++player) {
    cursors_[player] = 0;
    while (cursors_[player] + 1U < cellCount &&
           (lights_[player] & (1UL << cursors_[player])) == 0U) {
      ++cursors_[player];
    }
  }
  winner_ = 0;
  phase_ = Phase::Playing;
  phaseChangedAt_ = now;
}

void LightsOutGame::moveCursor(uint8_t player, int8_t delta,
                               uint8_t cellCount) {
  int16_t target = static_cast<int16_t>(cursors_[player]) + delta;
  while (target < 0) {
    target += cellCount;
  }
  while (target >= cellCount) {
    target -= cellCount;
  }
  cursors_[player] = static_cast<uint8_t>(target);
}

void LightsOutGame::toggleAt(uint8_t player, uint8_t cellCount) {
  lights_[player] =
      toggledMask(lights_[player], cursors_[player], cellCount);
}

void LightsOutGame::checkDuelResult(uint32_t now) {
  const bool player1Solved = lights_[0] == 0;
  const bool player2Solved = lights_[1] == 0;
  if (!player1Solved && !player2Solved) {
    return;
  }

  if (player1Solved != player2Solved) {
    winner_ = player1Solved ? 1U : 2U;
    uint16_t& winnerScore = scores_[winner_ - 1U];
    if (winnerScore < 999U) {
      ++winnerScore;
    }
  } else {
    winner_ = 0;
  }
  phase_ = Phase::RoundWon;
  phaseChangedAt_ = now;
}

void LightsOutGame::update(uint32_t now) {
  if (phase_ == Phase::GameOver) {
    if (InputManager::wasPressed(InputManager::Button::Red) ||
        InputManager::wasPressed(InputManager::Button::Green) ||
        InputManager::wasPressed(InputManager::Button::Blue) ||
        InputManager::wasPressed(InputManager::Button::Yellow)) {
      start(now, true);
    }
    return;
  }

  if (phase_ == Phase::Solved) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::LIGHTS_OUT_SUCCESS_MS) {
      if (level_ < 255U) {
        ++level_;
      }
      generatePuzzle(now);
    }
    return;
  }

  if (phase_ == Phase::RoundWon) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::LIGHTS_OUT_ROUND_MS) {
      if (scores_[0] >= Config::LIGHTS_OUT_DUEL_SCORE_TO_WIN ||
          scores_[1] >= Config::LIGHTS_OUT_DUEL_SCORE_TO_WIN) {
        phase_ = Phase::GameOver;
        phaseChangedAt_ = now;
      } else {
        if (level_ < 255U) {
          ++level_;
        }
        generatePuzzle(now);
      }
    }
    return;
  }

  const uint8_t cellCount = versus_ ? DUEL_CELL_COUNT : CELL_COUNT;
  moveCursor(0, InputManager::encoderDelta(InputManager::Encoder::Player1),
             cellCount);
  if (versus_) {
    moveCursor(1, InputManager::encoderDelta(InputManager::Encoder::Player2),
               cellCount);
  }

  if (InputManager::wasPressed(InputManager::Button::Green)) {
    lights_[0] = initialLights_[0];
  }
  if (versus_ &&
      InputManager::wasPressed(InputManager::Button::Yellow)) {
    lights_[1] = initialLights_[1];
  }

  const bool player1Toggled =
      InputManager::wasPressed(InputManager::Button::Red);
  const bool player2Toggled =
      versus_ && InputManager::wasPressed(InputManager::Button::Blue);
  if (player1Toggled) {
    toggleAt(0, cellCount);
  }
  if (player2Toggled) {
    toggleAt(1, cellCount);
  }

  if (versus_) {
    checkDuelResult(now);
  } else if (lights_[0] == 0) {
    if (scores_[0] < 999U) {
      ++scores_[0];
    }
    phase_ = Phase::Solved;
    phaseChangedAt_ = now;
  }
}

void LightsOutGame::render(uint32_t now) const {
  LedManager::clearStrip();
  if (phase_ == Phase::Solved) {
    uint16_t count = static_cast<uint16_t>(
        (static_cast<uint32_t>(now - phaseChangedAt_) * Config::LED_COUNT) /
        Config::LIGHTS_OUT_SUCCESS_MS);
    if (count > Config::LED_COUNT) {
      count = Config::LED_COUNT;
    }
    LedManager::setStripRange(0, count, Config::LIGHTS_OUT_SUCCESS_COLOR);
    return;
  }

  if (phase_ == Phase::RoundWon) {
    const uint16_t half = Config::LED_COUNT / 2U;
    if (winner_ == 0) {
      LedManager::setStripRange(0, Config::LED_COUNT,
                                Config::LIGHTS_OUT_TIE_COLOR);
    } else {
      LedManager::setStripRange(winner_ == 1U ? 0 : half, half,
                                Config::LIGHTS_OUT_SUCCESS_COLOR);
    }
    return;
  }

  if (phase_ == Phase::GameOver) {
    const bool bright = ((now - phaseChangedAt_) / 180U) % 2U == 0U;
    const uint16_t half = Config::LED_COUNT / 2U;
    const uint8_t gameWinner = scores_[0] > scores_[1] ? 1U : 2U;
    if (bright) {
      LedManager::setStripRange(gameWinner == 1U ? 0 : half, half,
                                gameWinner == 1U
                                    ? Config::LIGHTS_OUT_PLAYER_1_CURSOR_COLOR
                                    : Config::LIGHTS_OUT_PLAYER_2_CURSOR_COLOR);
    }
    return;
  }

  const uint8_t boardCount = versus_ ? 2U : 1U;
  const uint8_t cellCount = versus_ ? DUEL_CELL_COUNT : CELL_COUNT;
  for (uint8_t player = 0; player < boardCount; ++player) {
    const uint8_t cellOffset = player * cellCount;
    for (uint8_t cell = 0; cell < cellCount; ++cell) {
      const uint16_t start = static_cast<uint16_t>(cellOffset + cell) *
                             Config::GAME_PIXEL_WIDTH;
      if ((lights_[player] & (1UL << cell)) != 0U) {
        LedManager::setStripRange(start + 1U,
                                  Config::GAME_PIXEL_WIDTH - 2U,
                                  Config::LIGHTS_OUT_ON_COLOR);
      }
      if (cell == cursors_[player]) {
        const uint32_t cursorColor =
            !versus_ ? Config::LIGHTS_OUT_CURSOR_COLOR
                     : player == 0
                           ? Config::LIGHTS_OUT_PLAYER_1_CURSOR_COLOR
                           : Config::LIGHTS_OUT_PLAYER_2_CURSOR_COLOR;
        LedManager::setStripPixel(start, cursorColor);
        LedManager::setStripPixel(start + Config::GAME_PIXEL_WIDTH - 1U,
                                  cursorColor);
      }
    }
  }
}
