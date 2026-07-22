#include "games/whack_1d.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t ZONE_COUNT = 4;
constexpr uint16_t ZONE_WIDTH = Config::LED_COUNT / ZONE_COUNT;

static_assert(Config::LED_COUNT % ZONE_COUNT == 0,
              "Whack requires four equal color zones");

}  // namespace

void Whack1DGame::start(uint32_t now) {
  score_ = 0;
  lives_ = Config::WHACK_STARTING_LIVES;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(F("Whack started: hit each brightly lit color zone"));
  spawnWave(now);
}

uint16_t Whack1DGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

int8_t Whack1DGame::pressedZone() {
  if (InputManager::wasPressed(InputManager::Button::Red)) {
    return 0;
  }
  if (InputManager::wasPressed(InputManager::Button::Green)) {
    return 1;
  }
  if (InputManager::wasPressed(InputManager::Button::Blue)) {
    return 2;
  }
  if (InputManager::wasPressed(InputManager::Button::Yellow)) {
    return 3;
  }
  return -1;
}

uint32_t Whack1DGame::zoneColor(uint8_t zone, bool bright) {
  if (bright) {
    switch (zone) {
      case 0:
        return Config::BUTTON_1_COLOR;
      case 1:
        return Config::BUTTON_2_COLOR;
      case 2:
        return Config::BUTTON_3_COLOR;
      default:
        return Config::BUTTON_4_COLOR;
    }
  }
  switch (zone) {
    case 0:
      return Config::WHACK_DIM_RED;
    case 1:
      return Config::WHACK_DIM_GREEN;
    case 2:
      return Config::WHACK_DIM_BLUE;
    default:
      return Config::WHACK_DIM_YELLOW;
  }
}

uint8_t Whack1DGame::targetCount() const {
  if (score_ >= Config::WHACK_THREE_TARGET_SCORE) {
    return 3;
  }
  return score_ >= Config::WHACK_TWO_TARGET_SCORE ? 2U : 1U;
}

uint16_t Whack1DGame::waveDuration() const {
  const uint16_t reduction =
      score_ * Config::WHACK_SPEEDUP_MS >
              Config::WHACK_INITIAL_DURATION_MS -
                  Config::WHACK_MINIMUM_DURATION_MS
          ? Config::WHACK_INITIAL_DURATION_MS -
                Config::WHACK_MINIMUM_DURATION_MS
          : score_ * Config::WHACK_SPEEDUP_MS;
  return Config::WHACK_INITIAL_DURATION_MS - reduction;
}

void Whack1DGame::spawnWave(uint32_t now) {
  activeMask_ = 0;
  const uint8_t count = targetCount();
  while (__builtin_popcount(activeMask_) < count) {
    activeMask_ |= 1U << (nextRandom() & 0x03U);
  }
  waveStartedAt_ = now;
  phase_ = Phase::Playing;
}

void Whack1DGame::loseLife(uint32_t now) {
  if (lives_ > 0) {
    --lives_;
  }
  activeMask_ = 0;
  phaseChangedAt_ = now;
  phase_ = lives_ == 0 ? Phase::GameOver : Phase::Error;
}

void Whack1DGame::update(uint32_t now) {
  if (phase_ == Phase::GameOver) {
    if (pressedZone() >= 0) {
      start(now);
    }
    return;
  }
  if (phase_ == Phase::Error) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::WHACK_ERROR_MS) {
      spawnWave(now);
    }
    return;
  }

  if (static_cast<uint32_t>(now - waveStartedAt_) >= waveDuration()) {
    loseLife(now);
    return;
  }

  const int8_t zone = pressedZone();
  if (zone < 0) {
    return;
  }
  const uint8_t bit = 1U << zone;
  if ((activeMask_ & bit) == 0U) {
    loseLife(now);
    return;
  }

  activeMask_ &= ~bit;
  if (score_ < 999U) {
    ++score_;
  }
  if (activeMask_ == 0U) {
    spawnWave(now);
  }
}

void Whack1DGame::render(uint32_t now) const {
  LedManager::clearStrip();
  if (phase_ == Phase::Error) {
    LedManager::setStripRange(0, Config::LED_COUNT,
                              Config::WHACK_ERROR_COLOR);
    return;
  }
  if (phase_ == Phase::GameOver) {
    if (((now - phaseChangedAt_) / 180U) % 2U == 0U) {
      LedManager::setStripRange(0, Config::LED_COUNT,
                                Config::WHACK_ERROR_COLOR);
    }
    return;
  }

  const uint16_t duration = waveDuration();
  const uint32_t elapsed = now - waveStartedAt_;
  const uint16_t remaining =
      elapsed >= duration ? 0U : duration - static_cast<uint16_t>(elapsed);
  const uint8_t countdownWidth =
      Config::GAME_PIXEL_WIDTH +
      (static_cast<uint32_t>(Config::WHACK_TARGET_WIDTH -
                             Config::GAME_PIXEL_WIDTH) *
       remaining) /
          duration;

  for (uint8_t zone = 0; zone < ZONE_COUNT; ++zone) {
    const uint16_t start = static_cast<uint16_t>(zone) * ZONE_WIDTH;
    LedManager::setStripRange(start + 1U, ZONE_WIDTH - 2U,
                              zoneColor(zone, false));
    if ((activeMask_ & (1U << zone)) != 0U) {
      const uint16_t targetStart =
          start + (ZONE_WIDTH - countdownWidth) / 2U;
      LedManager::setStripRange(targetStart, countdownWidth,
                                zoneColor(zone, true));
      LedManager::setStripRange(
          start + (ZONE_WIDTH - Config::GAME_PIXEL_WIDTH) / 2U,
          Config::GAME_PIXEL_WIDTH, Config::WHACK_TARGET_CORE_COLOR);
    }
  }

  for (uint8_t life = 0; life < lives_; ++life) {
    LedManager::setStripPixel(life, Config::WHACK_LIFE_COLOR);
  }
}
