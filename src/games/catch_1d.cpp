#include "games/catch_1d.h"

#include "config.h"
#include "controls.h"
#include "input_manager.h"
#include "led_manager.h"

void Catch1DGame::start(uint32_t now) {
  score_ = 0;
  markerPosition_ = 0;
  direction_ = 1;
  stepIntervalMs_ = Config::gameplayInterval(Config::CATCH_INITIAL_STEP_MS);
  lastStepAt_ = now;
  phaseChangedAt_ = now;
  phase_ = Phase::Playing;
  DEBUG_PRINTLN(F("Catch 1D started: press Red inside the green target"));
}

bool Catch1DGame::markerInTarget() const {
  const uint16_t start = targetStart();
  return markerPosition_ >= start &&
         markerPosition_ < start + targetWidth();
}

uint8_t Catch1DGame::targetWidth() const {
  const uint16_t shrink =
      score_ * Config::CATCH_TARGET_SHRINK_PIXELS_PER_HIT;
  const uint8_t availableShrink =
      Config::CATCH_INITIAL_TARGET_WIDTH -
      Config::CATCH_MINIMUM_TARGET_WIDTH;
  if (shrink >= availableShrink) {
    return Config::CATCH_MINIMUM_TARGET_WIDTH;
  }
  return Config::CATCH_INITIAL_TARGET_WIDTH - static_cast<uint8_t>(shrink);
}

uint16_t Catch1DGame::targetStart() const {
  return Config::LED_COUNT / 2U - targetWidth() / 2U;
}

bool Catch1DGame::anyColorPressed() {
  return InputManager::wasPressed(InputManager::Button::Red) ||
         InputManager::wasPressed(InputManager::Button::Green) ||
         InputManager::wasPressed(InputManager::Button::Blue) ||
         InputManager::wasPressed(InputManager::Button::Yellow);
}

void Catch1DGame::moveMarker(uint32_t now) {
  uint8_t steps = static_cast<uint8_t>((now - lastStepAt_) / stepIntervalMs_);
  if (steps > Config::CATCH_MAX_CATCHUP_STEPS) {
    steps = Config::CATCH_MAX_CATCHUP_STEPS;
  }
  if (steps == 0) {
    return;
  }
  lastStepAt_ += static_cast<uint32_t>(steps) * stepIntervalMs_;

  while (steps-- > 0) {
    if (direction_ > 0) {
      if (markerPosition_ >= Config::LED_COUNT - 1U) {
        direction_ = -1;
        --markerPosition_;
      } else {
        ++markerPosition_;
      }
    } else if (markerPosition_ == 0) {
      direction_ = 1;
      ++markerPosition_;
    } else {
      --markerPosition_;
    }
  }
}

void Catch1DGame::update(uint32_t now) {
  if (phase_ == Phase::GameOver) {
    if (anyColorPressed()) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::HitEffect) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::CATCH_HIT_EFFECT_MS) {
      phase_ = Phase::Playing;
      lastStepAt_ = now;
    }
    return;
  }

  if (Controls::primaryPressed(Controls::Player::One)) {
    phaseChangedAt_ = now;
    if (!markerInTarget()) {
      phase_ = Phase::GameOver;
      return;
    }

    if (score_ < 999U) {
      ++score_;
    }
    const uint16_t minimum =
        Config::gameplayInterval(Config::CATCH_MINIMUM_STEP_MS);
    if (stepIntervalMs_ > minimum) {
      const uint16_t faster = stepIntervalMs_ - Config::CATCH_SPEEDUP_MS;
      stepIntervalMs_ = faster < minimum ? minimum : faster;
    }
    phase_ = Phase::HitEffect;
    return;
  }

  moveMarker(now);
}

void Catch1DGame::render(uint32_t now) const {
  LedManager::clearStrip();

  const uint8_t width = targetWidth();
  const uint16_t start = targetStart();
  const uint32_t targetColor = phase_ == Phase::HitEffect
                                   ? Config::CATCH_SUCCESS_COLOR
                                   : Config::CATCH_TARGET_COLOR;
  LedManager::setStripRange(start, width, targetColor);

  const uint8_t halfWidth = Config::CATCH_MARKER_WIDTH / 2U;
  for (uint8_t offset = 0; offset < Config::CATCH_MARKER_WIDTH; ++offset) {
    const int16_t pixel = static_cast<int16_t>(markerPosition_) + offset -
                          static_cast<int16_t>(halfWidth);
    if (pixel >= 0 && pixel < static_cast<int16_t>(Config::LED_COUNT)) {
      LedManager::setStripPixel(static_cast<uint16_t>(pixel),
                                Config::CATCH_MARKER_COLOR);
    }
  }

  if (phase_ == Phase::HitEffect) {
    const uint32_t elapsed = now - phaseChangedAt_;
    const uint8_t radius = static_cast<uint8_t>(
        1U + elapsed * Config::EXPLOSION_INTENSITY /
                 Config::CATCH_HIT_EFFECT_MS);
    const uint16_t center = Config::LED_COUNT / 2U;
    if (center >= radius) {
      LedManager::setStripPixel(center - radius,
                                Config::CATCH_SUCCESS_COLOR);
    }
    if (center + radius < Config::LED_COUNT) {
      LedManager::setStripPixel(center + radius,
                                Config::CATCH_SUCCESS_COLOR);
    }
  } else if (phase_ == Phase::GameOver && ((now / 160U) & 1U) == 0U) {
    LedManager::setStripRange(start, width, Config::CATCH_ERROR_COLOR);
  }
}
