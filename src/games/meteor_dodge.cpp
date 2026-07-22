#include "games/meteor_dodge.h"

#include "config.h"
#include "controls.h"
#include "led_manager.h"

namespace {

constexpr uint8_t CELL_COUNT =
    Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;
constexpr uint8_t FIRST_PLAY_CELL = 1;
constexpr uint8_t LAST_PLAY_CELL = CELL_COUNT - 2U;

}  // namespace

void MeteorDodgeGame::start(uint32_t now) {
  playerCell_ = CELL_COUNT / 2U;
  facing_ = 1;
  lives_ = Config::METEOR_STARTING_LIVES;
  shields_ = Config::METEOR_STARTING_SHIELDS;
  shieldActive_ = false;
  score_ = 0;
  lastDashAt_ = now - Config::METEOR_DASH_COOLDOWN_MS;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(F("Meteor Dodge started"));
  DEBUG_PRINTLN(F("P1 encoder moves, Red dashes, Green shields"));
  startMeteor(now);
}

uint16_t MeteorDodgeGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

void MeteorDodgeGame::startMeteor(uint32_t now) {
  meteorCell_ = FIRST_PLAY_CELL +
                nextRandom() % (LAST_PLAY_CELL - FIRST_PLAY_CELL + 1U);
  phaseChangedAt_ = now;
  phase_ = Phase::Warning;
}

void MeteorDodgeGame::move(int8_t direction, bool dash, uint32_t now) {
  facing_ = direction;
  if (dash && static_cast<uint32_t>(now - lastDashAt_) <
                  Config::METEOR_DASH_COOLDOWN_MS) {
    return;
  }

  const int8_t distance = dash ? Config::METEOR_DASH_CELLS : 1;
  int16_t target = static_cast<int16_t>(playerCell_) +
                   static_cast<int16_t>(direction) * distance;
  if (target < FIRST_PLAY_CELL) {
    target = FIRST_PLAY_CELL;
  } else if (target > LAST_PLAY_CELL) {
    target = LAST_PLAY_CELL;
  }
  playerCell_ = static_cast<uint8_t>(target);
  if (dash) {
    lastDashAt_ = now;
  }
}

void MeteorDodgeGame::resolveImpact(uint32_t now) {
  const uint8_t distance = playerCell_ > meteorCell_
                               ? playerCell_ - meteorCell_
                               : meteorCell_ - playerCell_;
  if (distance <= Config::METEOR_BLAST_RADIUS_CELLS) {
    if (shieldActive_) {
      shieldActive_ = false;
    } else if (lives_ > 0) {
      --lives_;
    }
  } else {
    ++score_;
  }

  phaseChangedAt_ = now;
  phase_ = lives_ == 0 ? Phase::GameOver : Phase::Impact;
}

void MeteorDodgeGame::update(uint32_t now) {
  int8_t movement = Controls::rotation(Controls::Player::One);
  if (phase_ == Phase::GameOver) {
    if (movement != 0 ||
        Controls::primaryPressed(Controls::Player::One) ||
        Controls::secondaryPressed(Controls::Player::One)) {
      start(now);
    }
    return;
  }

  while (movement < 0) {
    move(-1, false, now);
    ++movement;
  }
  while (movement > 0) {
    move(1, false, now);
    --movement;
  }
  if (Controls::primaryPressed(Controls::Player::One)) {
    move(facing_, true, now);
  }
  if (Controls::secondaryPressed(Controls::Player::One) &&
      !shieldActive_ && shields_ > 0) {
    --shields_;
    shieldActive_ = true;
  }

  if (phase_ == Phase::Impact) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::METEOR_IMPACT_MS) {
      startMeteor(now);
    }
    return;
  }

  const uint8_t speedSteps =
      score_ > Config::METEOR_MAX_SPEED_STEPS
          ? Config::METEOR_MAX_SPEED_STEPS
          : static_cast<uint8_t>(score_);
  const uint16_t warningMs =
      Config::METEOR_INITIAL_WARNING_MS -
      static_cast<uint16_t>(speedSteps) * Config::METEOR_WARNING_SPEEDUP_MS;
  if (static_cast<uint32_t>(now - phaseChangedAt_) >=
      Config::gameplayInterval(warningMs)) {
    resolveImpact(now);
  }
}

void MeteorDodgeGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::GameOver) {
    if ((now / 160U) % 2U == 0U) {
      LedManager::setStripRange(Config::LED_COUNT / 2U -
                                    Config::EXPLOSION_INTENSITY * 2U,
                                Config::EXPLOSION_INTENSITY * 4U,
                                Config::METEOR_BLAST_COLOR);
    }
    return;
  }

  if (phase_ == Phase::Warning) {
    const uint32_t warningColor =
        ((now / 100U) & 1U) == 0U ? Config::METEOR_WARNING_COLOR : 0x401000UL;
    LedManager::setGameCell(meteorCell_, warningColor);
  } else {
    const uint8_t start =
        meteorCell_ > Config::METEOR_BLAST_RADIUS_CELLS
            ? meteorCell_ - Config::METEOR_BLAST_RADIUS_CELLS
            : 0;
    const uint8_t end = meteorCell_ + Config::METEOR_BLAST_RADIUS_CELLS >=
                                CELL_COUNT
                            ? CELL_COUNT - 1U
                            : meteorCell_ + Config::METEOR_BLAST_RADIUS_CELLS;
    for (uint8_t cell = start; cell <= end; ++cell) {
      LedManager::setGameCell(
          cell, ((now / 35U + cell) & 1U) == 0U ? 0xFFFFFFUL
                                                : Config::METEOR_BLAST_COLOR);
    }
  }

  LedManager::setGameCell(
      playerCell_, shieldActive_ ? Config::METEOR_SHIELD_COLOR
                                 : Config::METEOR_PLAYER_COLOR);
  const uint16_t playerCenter =
      static_cast<uint16_t>(playerCell_) * Config::GAME_PIXEL_WIDTH +
      Config::GAME_PIXEL_WIDTH / 2U;
  LedManager::setStripPixel(playerCenter, Config::METEOR_PLAYER_COLOR);

  for (uint8_t life = 0; life < lives_; ++life) {
    LedManager::setStripPixel(life, Config::METEOR_LIFE_COLOR);
  }
  for (uint8_t shield = 0; shield < shields_; ++shield) {
    LedManager::setStripPixel(Config::LED_COUNT - 1U - shield,
                              Config::METEOR_SHIELD_COLOR);
  }
}
