#include "games/pong_1d.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

void Pong1DGame::start(uint32_t now) {
  leftScore_ = 0;
  rightScore_ = 0;
  nextServeDirection_ = 1;
  Serial.println(F("1D Pong started"));
  Serial.println(F("Button 1: left hit, Button 3: right hit, Button 4: pause"));
  serve(now);
}

void Pong1DGame::serve(uint32_t now) {
  ballPosition_ = Config::LED_COUNT / 2;
  ballDirection_ = nextServeDirection_;
  nextServeDirection_ = -nextServeDirection_;
  stepIntervalMs_ = Config::PONG_INITIAL_STEP_MS;
  lastStepAt_ = now;
  phase_ = Phase::Playing;
}

void Pong1DGame::handleHits() {
  if (InputManager::wasPressed(InputManager::Button::Game1) &&
      ballDirection_ < 0 && ballPosition_ < Config::PONG_HIT_ZONE_LENGTH) {
    ballDirection_ = 1;
    if (stepIntervalMs_ > Config::PONG_MINIMUM_STEP_MS) {
      stepIntervalMs_ -= Config::PONG_SPEEDUP_MS;
    }
    Serial.println(F("Left player hit"));
  }

  const uint16_t rightHitZoneStart =
      Config::LED_COUNT - Config::PONG_HIT_ZONE_LENGTH;
  if (InputManager::wasPressed(InputManager::Button::Game3) &&
      ballDirection_ > 0 && ballPosition_ >= rightHitZoneStart) {
    ballDirection_ = -1;
    if (stepIntervalMs_ > Config::PONG_MINIMUM_STEP_MS) {
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
    Serial.println(F(" player wins. Press encoder to restart."));
  } else {
    phase_ = Phase::PointPause;
  }
}

void Pong1DGame::printScore() const {
  Serial.print(F("Score "));
  Serial.print(leftScore_);
  Serial.print(F(" - "));
  Serial.println(rightScore_);
}

void Pong1DGame::update(uint32_t now) {
  if (InputManager::wasPressed(InputManager::Button::Game4) &&
      phase_ != Phase::GameOver && phase_ != Phase::PointPause) {
    if (phase_ == Phase::Paused) {
      phase_ = Phase::Playing;
      lastStepAt_ = now;
      Serial.println(F("Pong resumed"));
    } else {
      phase_ = Phase::Paused;
      Serial.println(F("Pong paused"));
    }
  }

  if (phase_ == Phase::Paused) {
    return;
  }

  if (phase_ == Phase::GameOver) {
    if (InputManager::wasPressed(InputManager::Button::EncoderClick)) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::PointPause) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::PONG_POINT_PAUSE_MS) {
      serve(now);
    }
    return;
  }

  handleHits();
  moveBall(now);
}

void Pong1DGame::render(uint32_t now) const {
  LedManager::clearStrip();

  for (uint8_t index = 0; index < Config::PONG_PADDLE_LENGTH; ++index) {
    LedManager::setStripPixel(index, Config::PONG_LEFT_PLAYER_COLOR);
    LedManager::setStripPixel(Config::LED_COUNT - 1 - index,
                              Config::PONG_RIGHT_PLAYER_COLOR);
  }

  if (phase_ == Phase::Paused && ((now / 300U) % 2U == 0U)) {
    return;
  }

  LedManager::setStripPixel(ballPosition_, Config::PONG_BALL_COLOR);
}
