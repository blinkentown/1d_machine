#include "games/colour_shooter.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

void ColourShooterGame::start(uint32_t now) {
  score_ = 0;
  lives_ = Config::COLOUR_SHOOTER_STARTING_LIVES;
  stepIntervalMs_ = Config::COLOUR_SHOOTER_INITIAL_STEP_MS;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }

  Serial.println(F("Colour Shooter started"));
  Serial.println(F("Match the incoming target with Red, Green, Blue, or Yellow"));
  spawnTarget(now);
}

uint8_t ColourShooterGame::nextColorIndex() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return static_cast<uint8_t>(randomState_ & 0x03U);
}

uint32_t ColourShooterGame::colorForIndex(uint8_t index) {
  switch (index) {
    case 0:
      return Config::BUTTON_1_COLOR;
    case 1:
      return Config::BUTTON_2_COLOR;
    case 2:
      return Config::BUTTON_3_COLOR;
    case 3:
      return Config::BUTTON_4_COLOR;
  }

  return 0;
}

int8_t ColourShooterGame::pressedColorIndex() {
  if (InputManager::wasPressed(InputManager::Button::Game1)) {
    return 0;
  }
  if (InputManager::wasPressed(InputManager::Button::Game2)) {
    return 1;
  }
  if (InputManager::wasPressed(InputManager::Button::Game3)) {
    return 2;
  }
  if (InputManager::wasPressed(InputManager::Button::Game4)) {
    return 3;
  }

  return -1;
}

void ColourShooterGame::spawnTarget(uint32_t now) {
  targetPosition_ = Config::LED_COUNT - 1;
  targetColorIndex_ = nextColorIndex();
  lastStepAt_ = now;
  phase_ = Phase::Playing;
}

void ColourShooterGame::beginFeedback(uint32_t now, uint16_t durationMs,
                                      uint32_t color) {
  phase_ = Phase::Feedback;
  phaseChangedAt_ = now;
  feedbackDurationMs_ = durationMs;
  feedbackColor_ = color;
}

void ColourShooterGame::loseLife(uint32_t now,
                                 const __FlashStringHelper* reason) {
  if (lives_ > 0) {
    --lives_;
  }

  Serial.print(reason);
  Serial.print(F(". Lives: "));
  Serial.println(lives_);

  if (lives_ == 0) {
    phase_ = Phase::GameOver;
    Serial.print(F("Game over. Score: "));
    Serial.println(score_);
    Serial.println(F("Press any color button to restart"));
  } else {
    beginFeedback(now, Config::COLOUR_SHOOTER_ERROR_FEEDBACK_MS,
                  Config::COLOUR_SHOOTER_ERROR_COLOR);
  }
}

void ColourShooterGame::handleButtonPress(uint32_t now) {
  const int8_t pressed = pressedColorIndex();
  if (pressed < 0) {
    return;
  }

  if (static_cast<uint8_t>(pressed) != targetColorIndex_) {
    loseLife(now, F("Wrong color"));
    return;
  }

  ++score_;
  Serial.print(F("Match. Score: "));
  Serial.println(score_);

  if (stepIntervalMs_ > Config::COLOUR_SHOOTER_MINIMUM_STEP_MS) {
    stepIntervalMs_ -= Config::COLOUR_SHOOTER_SPEEDUP_MS;
  }

  beginFeedback(now, Config::COLOUR_SHOOTER_CORRECT_FEEDBACK_MS,
                colorForIndex(targetColorIndex_));
}

void ColourShooterGame::handleMiss(uint32_t now) {
  loseLife(now, F("Target missed"));
}

void ColourShooterGame::update(uint32_t now) {
  if (phase_ == Phase::GameOver) {
    if (pressedColorIndex() >= 0) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::Feedback) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >= feedbackDurationMs_) {
      spawnTarget(now);
    }
    return;
  }

  handleButtonPress(now);
  if (phase_ != Phase::Playing) {
    return;
  }

  if (static_cast<uint32_t>(now - lastStepAt_) < stepIntervalMs_) {
    return;
  }
  lastStepAt_ = now;

  if (targetPosition_ == 0) {
    handleMiss(now);
  } else {
    --targetPosition_;
  }
}

void ColourShooterGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::Playing) {
    for (uint8_t index = 0; index < lives_; ++index) {
      LedManager::setStripPixel(index, Config::BUTTON_2_COLOR);
    }
    LedManager::setStripPixel(targetPosition_,
                              colorForIndex(targetColorIndex_));
  } else if (phase_ == Phase::Feedback) {
    LedManager::setStripPixel(Config::LED_COUNT / 2, feedbackColor_);
  } else if ((now / 300U) % 2U == 0U) {
    LedManager::setStripPixel(Config::LED_COUNT / 2 - 1,
                              Config::COLOUR_SHOOTER_ERROR_COLOR);
    LedManager::setStripPixel(Config::LED_COUNT / 2,
                              Config::COLOUR_SHOOTER_ERROR_COLOR);
    LedManager::setStripPixel(Config::LED_COUNT / 2 + 1,
                              Config::COLOUR_SHOOTER_ERROR_COLOR);
  }
}
