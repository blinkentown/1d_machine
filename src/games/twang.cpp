#include "games/twang.h"

#include "config.h"
#include "controls.h"
#include "input_manager.h"
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

  Serial.println(F("Twang started"));
  Serial.println(F("Red/Green move, Blue attacks, Yellow dashes"));
  startLevel(now);
}

void TwangGame::startLevel(uint32_t now) {
  playerCell_ = Config::TWANG_START_CELL;
  facing_ = 1;
  effectActive_ = false;
  lastDashAt_ = now - Config::TWANG_DASH_COOLDOWN_MS;
  generateDungeon();
  phase_ = Phase::Playing;
  phaseChangedAt_ = now;

  Serial.print(F("Twang level "));
  Serial.println(level_);
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
  for (uint8_t cell = Config::TWANG_START_CELL + 2U; cell < EXIT_CELL;
       ++cell) {
    const uint8_t roll = static_cast<uint8_t>(nextRandom() & 0x1FU);
    if (roll < 5U + density / 2U) {
      enemyMask_ |= cellBit(cell);
    } else if (roll < 8U + density / 2U) {
      lavaMask_ |= cellBit(cell);
    }
  }

  if (enemyMask_ == 0) {
    enemyMask_ |= cellBit(CELL_COUNT / 2U);
  }
}

void TwangGame::loseLife(uint32_t now,
                         const __FlashStringHelper* reason) {
  if (lives_ > 0) {
    --lives_;
  }
  effectCell_ = playerCell_;
  effectDirection_ = 0;
  effectStartedAt_ = now;
  effectActive_ = true;

  Serial.print(reason);
  Serial.print(F(". Lives: "));
  Serial.println(lives_);

  if (lives_ == 0) {
    phase_ = Phase::GameOver;
    phaseChangedAt_ = now;
    Serial.print(F("Twang game over. Score: "));
    Serial.println(score_);
    Serial.println(F("Press any color button to restart"));
  }
}

void TwangGame::move(int8_t direction, bool dash, uint32_t now) {
  facing_ = direction;
  if (dash && static_cast<uint32_t>(now - lastDashAt_) <
                  Config::TWANG_DASH_COOLDOWN_MS) {
    Serial.println(F("Dash recharging"));
    return;
  }

  const int8_t distance = dash ? Config::TWANG_DASH_CELLS : 1;
  const int16_t target = static_cast<int16_t>(playerCell_) +
                         static_cast<int16_t>(direction) * distance;
  if (target < Config::TWANG_START_CELL || target > EXIT_CELL) {
    return;
  }

  const uint8_t targetCell = static_cast<uint8_t>(target);
  const uint32_t targetBit = cellBit(targetCell);
  if ((enemyMask_ & targetBit) != 0U) {
    Serial.println(F("Enemy blocks the path"));
    return;
  }

  if ((lavaMask_ & targetBit) != 0U) {
    lavaMask_ &= ~targetBit;
    loseLife(now, F("Lava hit"));
    return;
  }

  effectCell_ = playerCell_;
  effectDirection_ = direction;
  effectStartedAt_ = now;
  effectActive_ = dash;
  playerCell_ = targetCell;
  if (dash) {
    lastDashAt_ = now;
  }

  if (playerCell_ == EXIT_CELL) {
    score_ += 10U + level_;
    phase_ = Phase::LevelClear;
    phaseChangedAt_ = now;
    Serial.print(F("Dungeon cleared. Score: "));
    Serial.println(score_);
  }
}

void TwangGame::attack(uint32_t now) {
  effectCell_ = playerCell_;
  effectDirection_ = facing_;
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
      effectCell_ = targetCell;
      ++score_;
      Serial.print(F("Enemy twanged. Score: "));
      Serial.println(score_);
      return;
    }
    if ((lavaMask_ & targetBit) != 0U) {
      break;
    }
  }
}

void TwangGame::update(uint32_t now) {
  if (effectActive_ &&
      static_cast<uint32_t>(now - effectStartedAt_) >=
          Config::TWANG_EFFECT_MS) {
    effectActive_ = false;
  }

  if (phase_ == Phase::GameOver) {
    if (InputManager::wasPressed(InputManager::Button::Red) ||
        InputManager::wasPressed(InputManager::Button::Green) ||
        InputManager::wasPressed(InputManager::Button::Blue) ||
        InputManager::wasPressed(InputManager::Button::Yellow)) {
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

  if (InputManager::wasPressed(Controls::ONE_PLAYER_LEFT)) {
    move(-1, false, now);
  }
  if (InputManager::wasPressed(Controls::ONE_PLAYER_RIGHT)) {
    move(1, false, now);
  }
  if (InputManager::wasPressed(Controls::ONE_PLAYER_ACTION)) {
    attack(now);
  }
  if (InputManager::wasPressed(Controls::ONE_PLAYER_SPECIAL)) {
    move(facing_, true, now);
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

  LedManager::setGameCell(playerCell_, Config::TWANG_PLAYER_COLOR);
  const uint16_t directionPixel =
      static_cast<uint16_t>(playerCell_) * Config::GAME_PIXEL_WIDTH +
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
