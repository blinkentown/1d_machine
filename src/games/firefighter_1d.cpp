#include "games/firefighter_1d.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t CELL_COUNT =
    Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;
constexpr uint8_t FIRST_PLAY_CELL = 1;
constexpr uint8_t LAST_PLAY_CELL = CELL_COUNT - 2U;

uint8_t cellDistance(uint8_t first, uint8_t second) {
  return first > second ? first - second : second - first;
}

}  // namespace

void Firefighter1DGame::start(uint32_t now) {
  score_ = 0;
  lives_ = Config::FIREFIGHTER_STARTING_LIVES;
  playerCell_ = CELL_COUNT / 2U;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  phase_ = Phase::Playing;
  sprayVisible_ = false;
  clearFires();
  fillFires(now);
  DEBUG_PRINTLN(F("Firefighter started: encoder moves, Blue sprays"));
}

uint16_t Firefighter1DGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

uint16_t Firefighter1DGame::burnDuration() const {
  const uint32_t possibleReduction =
      static_cast<uint32_t>(score_) * Config::FIREFIGHTER_SPEEDUP_MS;
  const uint16_t maximumReduction =
      Config::FIREFIGHTER_INITIAL_BURN_MS -
      Config::FIREFIGHTER_MINIMUM_BURN_MS;
  const uint16_t reduction =
      possibleReduction > maximumReduction
          ? maximumReduction
          : static_cast<uint16_t>(possibleReduction);
  return Config::FIREFIGHTER_INITIAL_BURN_MS - reduction;
}

uint8_t Firefighter1DGame::desiredFireCount() const {
  if (score_ >= Config::FIREFIGHTER_THREE_FIRE_SCORE) {
    return 3;
  }
  return score_ >= Config::FIREFIGHTER_TWO_FIRE_SCORE ? 2U : 1U;
}

void Firefighter1DGame::movePlayer(int8_t delta) {
  int16_t next = static_cast<int16_t>(playerCell_) + delta;
  if (next < FIRST_PLAY_CELL) {
    next = FIRST_PLAY_CELL;
  } else if (next > LAST_PLAY_CELL) {
    next = LAST_PLAY_CELL;
  }
  playerCell_ = static_cast<uint8_t>(next);
}

void Firefighter1DGame::clearFires() {
  for (Fire& fire : fires_) {
    fire.active = false;
  }
}

bool Firefighter1DGame::spawnFire(uint32_t now) {
  Fire* freeFire = nullptr;
  for (Fire& fire : fires_) {
    if (!fire.active) {
      freeFire = &fire;
      break;
    }
  }
  if (freeFire == nullptr) {
    return false;
  }

  uint8_t candidate = FIRST_PLAY_CELL;
  for (uint8_t attempt = 0; attempt < 24U; ++attempt) {
    candidate = FIRST_PLAY_CELL +
                nextRandom() % (LAST_PLAY_CELL - FIRST_PLAY_CELL + 1U);
    bool clear = cellDistance(candidate, playerCell_) >
                 Config::FIREFIGHTER_SPRAY_REACH_CELLS;
    for (const Fire& fire : fires_) {
      if (fire.active &&
          cellDistance(candidate, fire.cell) <=
              Config::FIREFIGHTER_MAX_FIRE_RADIUS_CELLS * 2U) {
        clear = false;
      }
    }
    if (clear) {
      break;
    }
  }

  freeFire->cell = candidate;
  freeFire->ignitedAt = now;
  freeFire->lifetime = burnDuration();
  freeFire->active = true;
  return true;
}

void Firefighter1DGame::fillFires(uint32_t now) {
  uint8_t activeCount = 0;
  for (const Fire& fire : fires_) {
    if (fire.active) {
      ++activeCount;
    }
  }
  while (activeCount < desiredFireCount() && spawnFire(now)) {
    ++activeCount;
  }
}

void Firefighter1DGame::spray(uint32_t now) {
  sprayStartedAt_ = now;
  sprayOriginCell_ = playerCell_;
  sprayTargetCell_ = playerCell_;
  sprayVisible_ = true;
  sprayHit_ = false;

  Fire* nearest = nullptr;
  uint8_t nearestDistance = 0xFFU;
  for (Fire& fire : fires_) {
    if (!fire.active) {
      continue;
    }
    const uint8_t distance = cellDistance(playerCell_, fire.cell);
    if (distance <= Config::FIREFIGHTER_SPRAY_REACH_CELLS &&
        distance < nearestDistance) {
      nearest = &fire;
      nearestDistance = distance;
    }
  }

  if (nearest == nullptr) {
    return;
  }
  sprayTargetCell_ = nearest->cell;
  nearest->active = false;
  sprayHit_ = true;
  if (score_ < 999U) {
    ++score_;
  }
  fillFires(now);
}

void Firefighter1DGame::loseLife(uint32_t now) {
  if (lives_ > 0U) {
    --lives_;
  }
  clearFires();
  sprayVisible_ = false;
  phaseChangedAt_ = now;
  phase_ = lives_ == 0U ? Phase::GameOver : Phase::Error;
}

void Firefighter1DGame::update(uint32_t now) {
  const int8_t movement =
      InputManager::encoderDelta(InputManager::Encoder::Player1);
  const bool bluePressed =
      InputManager::wasPressed(InputManager::Button::Blue);

  if (phase_ == Phase::GameOver) {
    if (movement != 0 || bluePressed) {
      start(now);
    }
    return;
  }
  if (phase_ == Phase::Error) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::FIREFIGHTER_ERROR_MS) {
      phase_ = Phase::Playing;
      fillFires(now);
    }
    return;
  }

  movePlayer(movement);
  if (bluePressed) {
    spray(now);
  }
  if (sprayVisible_ &&
      static_cast<uint32_t>(now - sprayStartedAt_) >=
          Config::FIREFIGHTER_SPRAY_MS) {
    sprayVisible_ = false;
  }

  for (const Fire& fire : fires_) {
    if (fire.active &&
        static_cast<uint32_t>(now - fire.ignitedAt) >= fire.lifetime) {
      loseLife(now);
      return;
    }
  }
}

void Firefighter1DGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::Error) {
    LedManager::setStripRange(0, Config::LED_COUNT,
                              Config::FIREFIGHTER_ERROR_COLOR);
    return;
  }
  if (phase_ == Phase::GameOver) {
    if (((now - phaseChangedAt_) / 160U) % 2U == 0U) {
      for (uint8_t cell = 0; cell < CELL_COUNT; ++cell) {
        LedManager::setGameCell(
            cell, (cell & 1U) == 0U ? Config::FIREFIGHTER_FIRE_COLOR
                                     : Config::FIREFIGHTER_ERROR_COLOR);
      }
    }
    return;
  }

  const uint16_t playerCenter =
      static_cast<uint16_t>(playerCell_) * Config::GAME_PIXEL_WIDTH +
      Config::GAME_PIXEL_WIDTH / 2U;
  const uint16_t reachPixels =
      static_cast<uint16_t>(Config::FIREFIGHTER_SPRAY_REACH_CELLS) *
      Config::GAME_PIXEL_WIDTH;
  const uint16_t rangeFirst =
      playerCenter > reachPixels ? playerCenter - reachPixels : 0U;
  const uint16_t rangeLast =
      playerCenter + reachPixels < Config::LED_COUNT
          ? playerCenter + reachPixels
          : Config::LED_COUNT - 1U;
  LedManager::setStripRange(rangeFirst, rangeLast - rangeFirst + 1U,
                            Config::FIREFIGHTER_WATER_RANGE_COLOR);
  LedManager::setStripPixel(rangeFirst, Config::FIREFIGHTER_WATER_COLOR);
  LedManager::setStripPixel(rangeLast, Config::FIREFIGHTER_WATER_COLOR);

  for (const Fire& fire : fires_) {
    if (!fire.active) {
      continue;
    }
    const uint32_t age = now - fire.ignitedAt;
    const uint16_t initialHalfWidth = Config::GAME_PIXEL_WIDTH / 2U;
    const uint16_t maximumHalfWidth =
        static_cast<uint16_t>(Config::FIREFIGHTER_MAX_FIRE_RADIUS_CELLS) *
            Config::GAME_PIXEL_WIDTH +
        Config::GAME_PIXEL_WIDTH / 2U;
    uint16_t halfWidth = initialHalfWidth +
                         (age * (maximumHalfWidth - initialHalfWidth)) /
                             fire.lifetime;
    if (halfWidth > maximumHalfWidth) {
      halfWidth = maximumHalfWidth;
    }
    const uint16_t center =
        static_cast<uint16_t>(fire.cell) * Config::GAME_PIXEL_WIDTH +
        Config::GAME_PIXEL_WIDTH / 2U;
    const uint16_t first = center > halfWidth ? center - halfWidth : 0U;
    const uint16_t last =
        center + halfWidth < Config::LED_COUNT
            ? center + halfWidth
            : Config::LED_COUNT - 1U;
    for (uint16_t pixel = first; pixel <= last; ++pixel) {
      const uint16_t distance = pixel > center ? pixel - center : center - pixel;
      const uint8_t flicker = static_cast<uint8_t>(
          ((now / 55U) + pixel * 7U + fire.cell * 11U) & 0x03U);
      uint32_t color;
      if (distance * 3U <= halfWidth) {
        color = flicker == 0U ? Config::FIREFIGHTER_PLAYER_CORE_COLOR
                              : Config::FIREFIGHTER_FIRE_CORE_COLOR;
      } else if (distance * 3U <= halfWidth * 2U) {
        color = flicker == 1U ? Config::FIREFIGHTER_FIRE_CORE_COLOR
                              : Config::FIREFIGHTER_FIRE_INNER_COLOR;
      } else {
        color = flicker == 2U ? Config::FIREFIGHTER_FIRE_INNER_COLOR
                              : Config::FIREFIGHTER_FIRE_COLOR;
      }
      LedManager::setStripPixel(pixel, color);
    }
  }

  if (sprayVisible_) {
    uint32_t elapsed = now - sprayStartedAt_;
    if (elapsed > Config::FIREFIGHTER_SPRAY_MS) {
      elapsed = Config::FIREFIGHTER_SPRAY_MS;
    }
    const int32_t origin =
        static_cast<int32_t>(sprayOriginCell_) * Config::GAME_PIXEL_WIDTH +
        Config::GAME_PIXEL_WIDTH / 2U;
    if (sprayHit_) {
      const int32_t target =
          static_cast<int32_t>(sprayTargetCell_) * Config::GAME_PIXEL_WIDTH +
          Config::GAME_PIXEL_WIDTH / 2U;
      const int32_t tip = origin +
                          ((target - origin) * static_cast<int32_t>(elapsed)) /
                              Config::FIREFIGHTER_SPRAY_MS;
      const uint16_t first =
          static_cast<uint16_t>(origin < tip ? origin : tip);
      const uint16_t last =
          static_cast<uint16_t>(origin > tip ? origin : tip);
      LedManager::setStripRange(first, last - first + 1U,
                                Config::FIREFIGHTER_WATER_COLOR);
      const uint16_t splashFirst = tip > Config::TAPE_PIXEL_WIDTH
                                       ? tip - Config::TAPE_PIXEL_WIDTH
                                       : 0U;
      const uint16_t splashLast =
          tip + Config::TAPE_PIXEL_WIDTH < Config::LED_COUNT
              ? tip + Config::TAPE_PIXEL_WIDTH
              : Config::LED_COUNT - 1U;
      LedManager::setStripRange(splashFirst,
                                splashLast - splashFirst + 1U,
                                Config::FIREFIGHTER_SPLASH_COLOR);
    } else {
      const uint16_t pulseRadius = static_cast<uint16_t>(
          (static_cast<uint32_t>(reachPixels) * elapsed) /
          Config::FIREFIGHTER_SPRAY_MS);
      const uint16_t first =
          origin > pulseRadius ? origin - pulseRadius : 0U;
      const uint16_t last =
          origin + pulseRadius < Config::LED_COUNT
              ? origin + pulseRadius
              : Config::LED_COUNT - 1U;
      LedManager::setStripPixel(first, Config::FIREFIGHTER_WATER_COLOR);
      LedManager::setStripPixel(last, Config::FIREFIGHTER_WATER_COLOR);
    }
  }

  LedManager::setStripRange(
      playerCenter - Config::TAPE_PIXEL_WIDTH / 2U,
      Config::TAPE_PIXEL_WIDTH, Config::FIREFIGHTER_PLAYER_COLOR);
  LedManager::setStripPixel(playerCenter,
                            Config::FIREFIGHTER_PLAYER_CORE_COLOR);
  for (uint8_t life = 0; life < lives_; ++life) {
    LedManager::setStripPixel(life, Config::FIREFIGHTER_LIFE_COLOR);
  }
}
