#include "games/twang.h"

#include "config.h"
#include "controls.h"
#include "led_manager.h"

namespace {

constexpr uint8_t CELL_COUNT =
    Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;
constexpr uint8_t EXIT_CELL = CELL_COUNT - 1U;

static_assert(Config::LED_COUNT % Config::GAME_PIXEL_WIDTH == 0,
              "Twang requires complete logical cells");
static_assert(CELL_COUNT <= 32, "Twang cell masks require at most 32 cells");

}  // namespace

void TwangGame::start(uint32_t now) {
  lives_ = Config::TWANG_STARTING_LIVES;
  level_ = 1;
  score_ = 0;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }

  DEBUG_PRINTLN(F("Twang started"));
  DEBUG_PRINTLN(F("P1 encoder moves, Red attacks, Green jumps"));
  startLevel(now);
}

void TwangGame::startLevel(uint32_t now) {
  playerCell_ = Config::TWANG_START_CELL;
  playerRenderPosition_ =
      static_cast<uint16_t>(playerCell_) * Config::GAME_PIXEL_WIDTH;
  lastPlayerPixelStepAt_ = now;
  facing_ = 1;
  effectActive_ = false;
  exitPending_ = false;
  generateDungeon();
  phase_ = Phase::Playing;
  phaseChangedAt_ = now;
}

uint16_t TwangGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

uint32_t TwangGame::cellBit(uint8_t cell) { return 1UL << cell; }

void TwangGame::generateDungeon() {
  enemyMask_ = 0;
  lavaMask_ = 0;

  const uint8_t density = level_ > 8U ? 8U : level_;
  bool reserveLandingCell = false;
  for (uint8_t cell = Config::TWANG_START_CELL + 2U; cell < EXIT_CELL;
       ++cell) {
    if (reserveLandingCell) {
      reserveLandingCell = false;
      continue;
    }
    const uint8_t roll = static_cast<uint8_t>(nextRandom() & 0x1FU);
    if (roll < 5U + density / 2U) {
      enemyMask_ |= cellBit(cell);
    } else if (roll < 8U + density / 2U) {
      lavaMask_ |= cellBit(cell);
      reserveLandingCell = true;
    }
  }

  if (enemyMask_ == 0) {
    for (uint8_t cell = Config::TWANG_START_CELL + 2U; cell < EXIT_CELL;
         ++cell) {
      const uint32_t bit = cellBit(cell);
      const uint32_t previousBit = cellBit(cell - 1U);
      if ((lavaMask_ & (bit | previousBit)) == 0U) {
        enemyMask_ |= bit;
        break;
      }
    }
  }
}

void TwangGame::loseLife(uint32_t now) {
  if (lives_ > 0) {
    --lives_;
  }
  effectCell_ = playerCell_;
  effectStartedAt_ = now;
  effectActive_ = true;

  if (lives_ == 0) {
    phase_ = Phase::GameOver;
    phaseChangedAt_ = now;
  }
}

bool TwangGame::move(int8_t direction, uint32_t now) {
  facing_ = direction;
  const int16_t target = static_cast<int16_t>(playerCell_) +
                         static_cast<int16_t>(direction);
  if (target < Config::TWANG_START_CELL || target > EXIT_CELL) {
    return false;
  }

  const uint8_t targetCell = static_cast<uint8_t>(target);
  const uint32_t targetBit = cellBit(targetCell);
  if ((enemyMask_ & targetBit) != 0U) {
    return false;
  }

  if ((lavaMask_ & targetBit) != 0U) {
    loseLife(now);
    return false;
  }

  playerCell_ = targetCell;

  if (playerCell_ == EXIT_CELL) {
    exitPending_ = true;
  }
  return true;
}

void TwangGame::jump(uint32_t now) {
  const int16_t obstacle = static_cast<int16_t>(playerCell_) + facing_;
  const int16_t landing = static_cast<int16_t>(playerCell_) +
                          static_cast<int16_t>(facing_) *
                              Config::TWANG_JUMP_CELLS;
  if (obstacle < Config::TWANG_START_CELL || obstacle >= EXIT_CELL ||
      landing < Config::TWANG_START_CELL || landing > EXIT_CELL) {
    return;
  }

  const uint32_t obstacleBit = cellBit(static_cast<uint8_t>(obstacle));
  const uint32_t landingBit = cellBit(static_cast<uint8_t>(landing));
  const bool obstaclePresent =
      ((enemyMask_ | lavaMask_) & obstacleBit) != 0U;
  const bool landingBlocked =
      ((enemyMask_ | lavaMask_) & landingBit) != 0U;
  if (!obstaclePresent || landingBlocked) {
    return;
  }

  playerCell_ = static_cast<uint8_t>(landing);
  effectCell_ = static_cast<uint8_t>(obstacle);
  effectStartedAt_ = now;
  effectActive_ = true;
  if (playerCell_ == EXIT_CELL) {
    exitPending_ = true;
  }
}

void TwangGame::updatePlayerAnimation(uint32_t now) {
  const uint16_t targetPosition =
      static_cast<uint16_t>(playerCell_) * Config::GAME_PIXEL_WIDTH;
  uint32_t dueSteps =
      (now - lastPlayerPixelStepAt_) / Config::TWANG_PIXEL_STEP_MS;
  if (dueSteps > Config::GAME_PIXEL_WIDTH * 2U) {
    dueSteps = Config::GAME_PIXEL_WIDTH * 2U;
  }
  uint8_t steps = static_cast<uint8_t>(dueSteps);
  if (steps > 0U) {
    lastPlayerPixelStepAt_ +=
        static_cast<uint32_t>(steps) * Config::TWANG_PIXEL_STEP_MS;
  }
  while (steps-- > 0U && playerRenderPosition_ != targetPosition) {
    playerRenderPosition_ +=
        playerRenderPosition_ < targetPosition ? 1 : -1;
  }

  if (exitPending_ && playerRenderPosition_ == targetPosition) {
    exitPending_ = false;
    phase_ = Phase::LevelClear;
    phaseChangedAt_ = now;
  }
}

void TwangGame::attack(uint32_t now) {
  effectCell_ = playerCell_;
  effectStartedAt_ = now;
  effectActive_ = true;

  for (uint8_t distance = 1; distance <= Config::TWANG_ATTACK_RANGE_CELLS;
       ++distance) {
    const int16_t target = static_cast<int16_t>(playerCell_) +
                           static_cast<int16_t>(facing_) * distance;
    if (target < Config::TWANG_START_CELL || target >= EXIT_CELL) {
      break;
    }

    const uint8_t targetCell = static_cast<uint8_t>(target);
    const uint32_t targetBit = cellBit(targetCell);
    if ((enemyMask_ & targetBit) != 0U) {
      enemyMask_ &= ~targetBit;
      if (score_ < 999U) {
        ++score_;
      }
      effectCell_ = targetCell;
      return;
    }
    if ((lavaMask_ & targetBit) != 0U) {
      break;
    }
  }
}

void TwangGame::update(uint32_t now) {
  updatePlayerAnimation(now);
  int8_t movement = Controls::rotation(Controls::Player::One);
  if (effectActive_ &&
      static_cast<uint32_t>(now - effectStartedAt_) >=
          Config::TWANG_EFFECT_MS) {
    effectActive_ = false;
  }

  if (phase_ == Phase::GameOver) {
    if (movement != 0 ||
        Controls::primaryPressed(Controls::Player::One) ||
        Controls::secondaryPressed(Controls::Player::One)) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::LevelClear) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::TWANG_LEVEL_CLEAR_MS) {
      if (level_ < 255U) {
        ++level_;
      }
      startLevel(now);
    }
    return;
  }

  while (movement < 0) {
    if (!move(-1, now)) {
      break;
    }
    ++movement;
  }
  while (movement > 0) {
    if (!move(1, now)) {
      break;
    }
    --movement;
  }
  if (phase_ != Phase::Playing) {
    return;
  }

  if (exitPending_) {
    return;
  }
  if (Controls::primaryPressed(Controls::Player::One)) {
    attack(now);
  }
  if (Controls::secondaryPressed(Controls::Player::One)) {
    jump(now);
  }
}

void TwangGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::GameOver) {
    if ((now / 180U) % 2U == 0U) {
      const uint8_t center = CELL_COUNT / 2U;
      LedManager::setGameCell(center - 1U, Config::TWANG_ENEMY_COLOR);
      LedManager::setGameCell(center, 0xFFFFFFUL);
      LedManager::setGameCell(center + 1U, Config::TWANG_ENEMY_COLOR);
    }
    return;
  }

  if (phase_ == Phase::LevelClear) {
    uint8_t cells = static_cast<uint8_t>(
        (static_cast<uint32_t>(now - phaseChangedAt_) * CELL_COUNT) /
        Config::TWANG_LEVEL_CLEAR_MS);
    if (cells > CELL_COUNT) {
      cells = CELL_COUNT;
    }
    for (uint8_t cell = 0; cell < cells; ++cell) {
      LedManager::setGameCell(cell, Config::TWANG_EXIT_COLOR);
    }
    return;
  }

  LedManager::setGameCell(EXIT_CELL, Config::TWANG_EXIT_COLOR);
  for (uint8_t cell = Config::TWANG_START_CELL; cell < EXIT_CELL; ++cell) {
    const uint32_t bit = cellBit(cell);
    if ((lavaMask_ & bit) != 0U) {
      LedManager::setGameCell(cell, ((now / 90U + cell) & 0x01U) == 0U
                                       ? Config::TWANG_LAVA_COLOR
                                       : 0x401000UL);
    } else if ((enemyMask_ & bit) != 0U) {
      LedManager::setGameCell(cell, ((now / 140U + cell) & 0x01U) == 0U
                                       ? Config::TWANG_ENEMY_COLOR
                                       : 0x500000UL);
    }
  }

  LedManager::setStripRange(playerRenderPosition_, Config::GAME_PIXEL_WIDTH,
                            Config::TWANG_PLAYER_COLOR);
  const uint16_t directionPixel =
      playerRenderPosition_ +
      (facing_ > 0 ? Config::GAME_PIXEL_WIDTH - 1U : 0U);
  LedManager::setStripPixel(directionPixel, Config::TWANG_ATTACK_COLOR);

  if (effectActive_) {
    const uint32_t elapsed = now - effectStartedAt_;
    const uint8_t radius = 1U + static_cast<uint8_t>(
                                   elapsed * Config::EXPLOSION_INTENSITY /
                                   Config::TWANG_EFFECT_MS);
    const uint16_t center =
        static_cast<uint16_t>(effectCell_) * Config::GAME_PIXEL_WIDTH +
        Config::GAME_PIXEL_WIDTH / 2U;
    for (uint8_t offset = 0; offset < radius; ++offset) {
      if (center >= offset) {
        LedManager::setStripPixel(center - offset,
                                  (offset & 1U) == 0U
                                      ? 0xFFFFFFUL
                                      : Config::TWANG_ATTACK_COLOR);
      }
      if (center + offset < Config::LED_COUNT) {
        LedManager::setStripPixel(center + offset,
                                  (offset & 1U) == 0U
                                      ? 0xFFFFFFUL
                                      : Config::TWANG_ATTACK_COLOR);
      }
    }
  }

  for (uint8_t life = 0; life < lives_; ++life) {
    LedManager::setStripPixel(life, Config::TWANG_LIFE_COLOR);
  }
}
