#include "games/colour_gate.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint16_t GATE_START =
    Config::COLOUR_GATE_CENTER - Config::COLOUR_GATE_WIDTH / 2U;
constexpr uint16_t GATE_END = GATE_START + Config::COLOUR_GATE_WIDTH - 1U;

}  // namespace

void ColourGateGame::start(uint32_t now) {
  score_ = 0;
  lives_ = Config::COLOUR_GATE_STARTING_LIVES;
  stepIntervalMs_ =
      Config::gameplayInterval(Config::COLOUR_GATE_INITIAL_STEP_MS);
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(F("Colour Gate started: match R/G/B/Y inside the gate"));
  spawnCue(now);
}

uint16_t ColourGateGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

uint32_t ColourGateGame::colorFor(uint8_t color) {
  switch (color) {
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

int8_t ColourGateGame::pressedColor() {
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

bool ColourGateGame::anyColorPressed() { return pressedColor() >= 0; }

void ColourGateGame::spawnCue(uint32_t now) {
  cuePosition_ = Config::LED_COUNT - 1U;
  cueColor_ = static_cast<uint8_t>(nextRandom() & 0x03U);
  lastStepAt_ = now;
  phaseChangedAt_ = now;
  phase_ = Phase::Playing;
}

bool ColourGateGame::cueInGate() const {
  return cuePosition_ >= GATE_START && cuePosition_ <= GATE_END;
}

void ColourGateGame::resolveCue(bool success, uint32_t now) {
  lastCueSucceeded_ = success;
  phaseChangedAt_ = now;
  if (success) {
    if (score_ < 999U) {
      ++score_;
    }
    if (score_ % Config::COLOUR_GATE_SPEEDUP_EVERY == 0U) {
      const uint16_t minimum =
          Config::gameplayInterval(Config::COLOUR_GATE_MINIMUM_STEP_MS);
      if (stepIntervalMs_ > minimum) {
        const uint16_t faster =
            stepIntervalMs_ - Config::COLOUR_GATE_SPEEDUP_MS;
        stepIntervalMs_ = faster < minimum ? minimum : faster;
      }
    }
    phase_ = Phase::Feedback;
    return;
  }

  if (lives_ > 0) {
    --lives_;
  }
  phase_ = lives_ == 0 ? Phase::GameOver : Phase::Feedback;
}

void ColourGateGame::moveCue(uint32_t now) {
  uint8_t steps = static_cast<uint8_t>((now - lastStepAt_) / stepIntervalMs_);
  if (steps > Config::COLOUR_GATE_MAX_CATCHUP_STEPS) {
    steps = Config::COLOUR_GATE_MAX_CATCHUP_STEPS;
  }
  if (steps == 0) {
    return;
  }
  lastStepAt_ += static_cast<uint32_t>(steps) * stepIntervalMs_;
  cuePosition_ = steps >= cuePosition_ ? 0U : cuePosition_ - steps;
}

void ColourGateGame::update(uint32_t now) {
  if (phase_ == Phase::GameOver) {
    if (anyColorPressed()) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::Feedback) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::COLOUR_GATE_FEEDBACK_MS) {
      spawnCue(now);
    }
    return;
  }

  const int8_t pressed = pressedColor();
  if (pressed >= 0) {
    resolveCue(cueInGate() && static_cast<uint8_t>(pressed) == cueColor_, now);
    return;
  }

  moveCue(now);
  if (cuePosition_ < GATE_START) {
    resolveCue(false, now);
  }
}

void ColourGateGame::render(uint32_t now) const {
  LedManager::clearStrip();

  uint32_t gateColor = Config::COLOUR_GATE_COLOR;
  if (phase_ == Phase::Feedback) {
    gateColor = lastCueSucceeded_ ? Config::COLOUR_GATE_SUCCESS_COLOR
                                  : Config::COLOUR_GATE_ERROR_COLOR;
  } else if (phase_ == Phase::GameOver) {
    gateColor = ((now / 160U) & 1U) == 0U
                    ? Config::COLOUR_GATE_ERROR_COLOR
                    : 0;
  }
  LedManager::setStripRange(GATE_START, Config::COLOUR_GATE_WIDTH, gateColor);

  if (phase_ == Phase::Playing) {
    for (uint8_t width = 0; width < Config::COLOUR_GATE_CUE_WIDTH; ++width) {
      if (cuePosition_ >= width) {
        LedManager::setStripPixel(cuePosition_ - width,
                                  colorFor(cueColor_));
      }
    }
  }

  for (uint8_t life = 0; life < lives_; ++life) {
    LedManager::setStripPixel(life, Config::COLOUR_GATE_LIFE_COLOR);
  }
}
