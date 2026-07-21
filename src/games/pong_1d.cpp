#include "games/pong_1d.h"

#include "config.h"
#include "controls.h"
#include "input_manager.h"
#include "led_manager.h"

void Pong1DGame::start(uint32_t now) {
  leftScore_ = 0;
  rightScore_ = 0;
  nextServeDirection_ = 1;
  perfectHitSide_ = 0;
  serveIntervalMs_ =
      Config::gameplayInterval(Config::PONG_INITIAL_STEP_MS);
  DEBUG_PRINTLN(F("1D Pong started"));
  DEBUG_PRINTLN(F("Red/P1-A: left hit, Blue/P2-A: right hit"));
  DEBUG_PRINTLN(F("Deeper hits return the ball faster"));
  serve(now);
}

void Pong1DGame::serve(uint32_t now) {
  ballPosition_ = Config::LED_COUNT / 2;
  ballDirection_ = nextServeDirection_;
  nextServeDirection_ = -nextServeDirection_;
  stepIntervalMs_ = serveIntervalMs_;
  lastStepAt_ = now;
  phase_ = Phase::Playing;
}

void Pong1DGame::handleHits(uint32_t now) {
  constexpr uint8_t hitZoneWidth =
      Config::PONG_HIT_ZONE_LENGTH * Config::GAME_PIXEL_WIDTH;

  if (InputManager::wasPressed(Controls::PLAYER_1_PRIMARY) &&
      ballDirection_ < 0 && ballPosition_ < hitZoneWidth) {
    const uint8_t depth = hitZoneWidth - 1U - ballPosition_;
    ballDirection_ = 1;
    applyHit(depth, -1, now);
  }

  const uint16_t rightHitZoneStart =
      Config::LED_COUNT - hitZoneWidth;
  if (InputManager::wasPressed(Controls::PLAYER_2_PRIMARY) &&
      ballDirection_ > 0 && ballPosition_ >= rightHitZoneStart) {
    const uint8_t depth = ballPosition_ - rightHitZoneStart;
    ballDirection_ = -1;
    applyHit(depth, 1, now);
  }
}

void Pong1DGame::applyHit(uint8_t depth, int8_t playerSide, uint32_t now) {
  constexpr uint8_t hitZoneWidth =
      Config::PONG_HIT_ZONE_LENGTH * Config::GAME_PIXEL_WIDTH;
  constexpr uint8_t bandWidth =
      hitZoneWidth / Config::PONG_HIT_QUALITY_BANDS;
  uint8_t quality = depth / bandWidth + 1U;
  if (quality > Config::PONG_HIT_QUALITY_BANDS) {
    quality = Config::PONG_HIT_QUALITY_BANDS;
  }

  const uint16_t minimumInterval =
      Config::gameplayInterval(Config::PONG_MINIMUM_STEP_MS);
  const uint8_t speedup = Config::PONG_SPEEDUP_MS * quality;
  if (stepIntervalMs_ > minimumInterval) {
    const uint16_t availableSpeedup = stepIntervalMs_ - minimumInterval;
    stepIntervalMs_ -= speedup < availableSpeedup ? speedup : availableSpeedup;
  }

  DEBUG_PRINT(playerSide < 0 ? F("Left") : F("Right"));
  DEBUG_PRINT(F(" player hit, quality "));
  DEBUG_PRINT(quality);
  DEBUG_PRINT(F("/"));
  DEBUG_PRINT(Config::PONG_HIT_QUALITY_BANDS);
  DEBUG_PRINT(F(", interval "));
  DEBUG_PRINT(stepIntervalMs_);
  DEBUG_PRINTLN(F(" ms"));

  if (quality == Config::PONG_HIT_QUALITY_BANDS) {
    perfectHitAt_ = now;
    perfectHitSide_ = playerSide;
    DEBUG_PRINTLN(F("Perfect hit!"));
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
    DEBUG_PRINTLN(F("Point: left player"));
  } else {
    ++rightScore_;
    DEBUG_PRINTLN(F("Point: right player"));
  }
  printScore();

  const uint16_t minimumServeInterval =
      Config::gameplayInterval(Config::PONG_MINIMUM_SERVE_STEP_MS);
  if (serveIntervalMs_ > minimumServeInterval) {
    const uint16_t availableSpeedup =
        serveIntervalMs_ - minimumServeInterval;
    serveIntervalMs_ -= Config::PONG_SERVE_SPEEDUP_MS < availableSpeedup
                            ? Config::PONG_SERVE_SPEEDUP_MS
                            : availableSpeedup;
  }

  phaseChangedAt_ = now;
  if (leftScore_ >= Config::PONG_WINNING_SCORE ||
      rightScore_ >= Config::PONG_WINNING_SCORE) {
    phase_ = Phase::GameOver;
    DEBUG_PRINT(leftScore_ > rightScore_ ? F("Left") : F("Right"));
    DEBUG_PRINTLN(F(" player wins. Press any color button to restart."));
  } else {
    phase_ = Phase::PointDelay;
  }
}

void Pong1DGame::printScore() const {
  DEBUG_PRINT(F("Score "));
  DEBUG_PRINT(leftScore_);
  DEBUG_PRINT(F(" - "));
  DEBUG_PRINTLN(rightScore_);
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

  handleHits(now);
  moveBall(now);
}

void Pong1DGame::renderPerfectHit(uint32_t now) const {
  if (perfectHitSide_ == 0) {
    return;
  }

  const uint32_t elapsed = now - perfectHitAt_;
  if (elapsed >= Config::PONG_PERFECT_HIT_EXPLOSION_MS) {
    return;
  }

  const uint8_t radius = 1U + static_cast<uint8_t>(
                                   elapsed * Config::PONG_PERFECT_HIT_RADIUS /
                                   Config::PONG_PERFECT_HIT_EXPLOSION_MS);
  const uint8_t frame = elapsed / Config::PONG_PERFECT_HIT_STROBE_MS;
  const bool whiteStrobe = (frame & 1U) == 0U;
  const uint32_t playerColor = perfectHitSide_ < 0
                                   ? Config::PONG_LEFT_PLAYER_COLOR
                                   : Config::PONG_RIGHT_PLAYER_COLOR;

  for (uint8_t offset = 0; offset < radius; ++offset) {
    const uint16_t pixel = perfectHitSide_ < 0
                               ? offset
                               : Config::LED_COUNT - 1U - offset;
    if (whiteStrobe || ((offset + frame) % 4U) == 0U) {
      LedManager::setStripPixel(pixel,
                                whiteStrobe ? Config::PONG_BALL_COLOR
                                            : playerColor);
    }
  }
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

  renderPerfectHit(now);
}
