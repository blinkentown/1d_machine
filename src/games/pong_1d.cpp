#include "games/pong_1d.h"

#include "config.h"
#include "controls.h"
#include "input_manager.h"
#include "led_manager.h"

void Pong1DGame::start(uint32_t now) {
  leftScore_ = 0;
  rightScore_ = 0;
  nextServeDirection_ = 1;
  Serial.println(F("1D Pong started"));
  Serial.println(F("Red/P1-A: left hit, Blue/P2-A: right hit"));
  serve(now);
}

void Pong1DGame::serve(uint32_t now) {
  ballPosition_ = Config::LED_COUNT / 2;
  ballDirection_ = nextServeDirection_;
  nextServeDirection_ = -nextServeDirection_;
  stepIntervalMs_ = Config::gameplayInterval(Config::PONG_INITIAL_STEP_MS);
  lastStepAt_ = now;
  phase_ = Phase::Playing;
}

void Pong1DGame::handleHits() {
  if (InputManager::wasPressed(Controls::PLAYER_1_PRIMARY) &&
      ballDirection_ < 0 &&
      ballPosition_ <
          Config::PONG_HIT_ZONE_LENGTH * Config::GAME_PIXEL_WIDTH) {
    ballDirection_ = 1;
    if (stepIntervalMs_ >
        Config::gameplayInterval(Config::PONG_MINIMUM_STEP_MS)) {
      stepIntervalMs_ -= Config::PONG_SPEEDUP_MS;
    }
    Serial.println(F("Left player hit"));
  }

  const uint16_t rightHitZoneStart =
      Config::LED_COUNT -
      Config::PONG_HIT_ZONE_LENGTH * Config::GAME_PIXEL_WIDTH;
  if (InputManager::wasPressed(Controls::PLAYER_2_PRIMARY) &&
      ballDirection_ > 0 && ballPosition_ >= rightHitZoneStart) {
    ballDirection_ = -1;
    if (stepIntervalMs_ >
        Config::gameplayInterval(Config::PONG_MINIMUM_STEP_MS)) {
      stepIntervalMs_ -= Config::PONG_SPEEDUP_MS;
    }
    Serial.println(F("Right player hit"));
  }
}

void Pong1DGame::moveBall(uint32_t now) {
  if (static_cast<uint32_t>(now - lastStepAt_) < stepIntervalMs_) {
    return;
  }
  lastStepAt_ = now;

  if (ballDirection_ < 0) {
    if (ballPosition_ == 0) {
      awardPoint(false, now);
    } else {
      --ballPosition_;
    }
  } else if (ballPosition_ >= Config::LED_COUNT - 1) {
    awardPoint(true, now);
  } else {
    ++ballPosition_;
  }
}

void Pong1DGame::awardPoint(bool leftPlayerScored, uint32_t now) {
  if (leftPlayerScored) {
    ++leftScore_;
    Serial.println(F("Point: left player"));
  } else {
    ++rightScore_;
    Serial.println(F("Point: right player"));
  }
  printScore();

  phaseChangedAt_ = now;
  if (leftScore_ >= Config::PONG_WINNING_SCORE ||
      rightScore_ >= Config::PONG_WINNING_SCORE) {
    phase_ = Phase::GameOver;
    Serial.print(leftScore_ > rightScore_ ? F("Left") : F("Right"));
    Serial.println(F(" player wins. Press any color button to restart."));
  } else {
    phase_ = Phase::PointDelay;
  }
}

void Pong1DGame::printScore() const {
  Serial.print(F("Score "));
  Serial.print(leftScore_);
  Serial.print(F(" - "));
  Serial.println(rightScore_);
}

void Pong1DGame::update(uint32_t now) {
  if (phase_ == Phase::GameOver) {
    if (InputManager::wasPressed(InputManager::Button::Red) ||
        InputManager::wasPressed(InputManager::Button::Green) ||
        InputManager::wasPressed(InputManager::Button::Blue) ||
        InputManager::wasPressed(InputManager::Button::Yellow) ||
        InputManager::wasPressed(InputManager::Button::EncoderClick)) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::PointDelay) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::PONG_POINT_DELAY_MS) {
      serve(now);
    }
    return;
  }

  handleHits();
  moveBall(now);
}

void Pong1DGame::render(uint32_t now) const {
  LedManager::clearStrip();

  const uint8_t paddleWidth =
      Config::PONG_PADDLE_LENGTH * Config::GAME_PIXEL_WIDTH;

  if (phase_ == Phase::GameOver) {
    if ((now / 250U) % 2U == 0U) {
      for (uint8_t index = 0; index < paddleWidth; ++index) {
        if (leftScore_ > rightScore_) {
          LedManager::setStripPixel(index, Config::PONG_LEFT_PLAYER_COLOR);
        } else {
          LedManager::setStripPixel(Config::LED_COUNT - 1 - index,
                                    Config::PONG_RIGHT_PLAYER_COLOR);
        }
      }
    }
    return;
  }

  for (uint8_t index = 0; index < paddleWidth; ++index) {
    LedManager::setStripPixel(index, Config::PONG_LEFT_PLAYER_COLOR);
    LedManager::setStripPixel(Config::LED_COUNT - 1 - index,
                              Config::PONG_RIGHT_PLAYER_COLOR);
  }

  for (uint8_t width = 0; width < Config::GAME_PIXEL_WIDTH; ++width) {
    if (ballDirection_ < 0) {
      LedManager::setStripPixel(ballPosition_ + width,
                                Config::PONG_BALL_COLOR);
    } else if (ballPosition_ >= width) {
      LedManager::setStripPixel(ballPosition_ - width,
                                Config::PONG_BALL_COLOR);
    }
  }
}
