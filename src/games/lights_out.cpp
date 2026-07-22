#include "games/lights_out.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t CELL_COUNT =
    Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;

static_assert(Config::LED_COUNT % Config::GAME_PIXEL_WIDTH == 0,
              "Lights Out requires complete logical cells");
static_assert(CELL_COUNT <= 32, "Lights Out uses a 32-bit cell mask");

}  // namespace

void LightsOutGame::start(uint32_t now) {
  score_ = 0;
  level_ = 1;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(F("Lights Out started: Red/Green move, Blue toggles, Yellow resets"));
  generatePuzzle(now);
}

uint16_t LightsOutGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

void LightsOutGame::toggleAt(uint8_t cell) {
  lights_ ^= 1UL << cell;
  if (cell > 0) {
    lights_ ^= 1UL << (cell - 1U);
  }
  if (cell + 1U < CELL_COUNT) {
    lights_ ^= 1UL << (cell + 1U);
  }
}

void LightsOutGame::generatePuzzle(uint32_t now) {
  lights_ = 0;
  const uint8_t extra =
      level_ < Config::LIGHTS_OUT_MAX_EXTRA_SCRAMBLES
          ? level_
          : Config::LIGHTS_OUT_MAX_EXTRA_SCRAMBLES;
  const uint8_t scrambleCount =
      Config::LIGHTS_OUT_BASE_SCRAMBLES + extra;

  uint8_t previous = CELL_COUNT;
  for (uint8_t index = 0; index < scrambleCount; ++index) {
    uint8_t cell = static_cast<uint8_t>(nextRandom() % CELL_COUNT);
    if (cell == previous) {
      cell = static_cast<uint8_t>((cell + 1U) % CELL_COUNT);
    }
    toggleAt(cell);
    previous = cell;
  }
  if (lights_ == 0) {
    toggleAt(static_cast<uint8_t>(nextRandom() % CELL_COUNT));
  }

  initialLights_ = lights_;
  cursor_ = 0;
  while (cursor_ + 1U < CELL_COUNT &&
         (lights_ & (1UL << cursor_)) == 0U) {
    ++cursor_;
  }
  phase_ = Phase::Playing;
  phaseChangedAt_ = now;
}

void LightsOutGame::update(uint32_t now) {
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

  if (InputManager::wasPressed(InputManager::Button::Red)) {
    cursor_ = cursor_ == 0 ? CELL_COUNT - 1U : cursor_ - 1U;
  }
  if (InputManager::wasPressed(InputManager::Button::Green)) {
    cursor_ = cursor_ + 1U >= CELL_COUNT ? 0 : cursor_ + 1U;
  }
  if (InputManager::wasPressed(InputManager::Button::Yellow)) {
    lights_ = initialLights_;
  }
  if (!InputManager::wasPressed(InputManager::Button::Blue)) {
    return;
  }

  toggleAt(cursor_);
  if (lights_ == 0) {
    if (score_ < 999U) {
      ++score_;
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

  for (uint8_t cell = 0; cell < CELL_COUNT; ++cell) {
    const uint16_t start =
        static_cast<uint16_t>(cell) * Config::GAME_PIXEL_WIDTH;
    if ((lights_ & (1UL << cell)) != 0U) {
      LedManager::setStripRange(start + 1U, Config::GAME_PIXEL_WIDTH - 2U,
                                Config::LIGHTS_OUT_ON_COLOR);
    }
    if (cell == cursor_) {
      LedManager::setStripPixel(start, Config::LIGHTS_OUT_CURSOR_COLOR);
      LedManager::setStripPixel(start + Config::GAME_PIXEL_WIDTH - 1U,
                                Config::LIGHTS_OUT_CURSOR_COLOR);
    }
  }
}
