#include "games/tennis_1d.h"

#include "config.h"
#include "controls.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint16_t PLAYER_2_COURT_START =
    Config::LED_COUNT - Config::TENNIS_COURT_LENGTH;
constexpr uint8_t HALF_PADDLE = Config::TENNIS_PADDLE_WIDTH / 2U;

uint8_t magnitude(int8_t value) {
  return static_cast<uint8_t>(value < 0 ? -value : value);
}

}  // namespace

void Tennis1DGame::start(uint32_t now) {
  player1Score_ = 0;
  player2Score_ = 0;
  serveFromPlayer1_ = true;
  lastHitPower_ = 0;
  player1SwingVelocity_ = 0;
  player2SwingVelocity_ = 0;
  resetPaddles();
  phase_ = Phase::ServeDelay;
  phaseChangedAt_ = now;
  DEBUG_PRINTLN(F("Tennis 1D started"));
  DEBUG_PRINTLN(F("Both encoders move; swing inward through the ball"));
}

void Tennis1DGame::resetPaddles() {
  player1Paddle_ = Config::TENNIS_COURT_LENGTH / 2U;
  player2Paddle_ = PLAYER_2_COURT_START +
                   Config::TENNIS_COURT_LENGTH / 2U;
}

void Tennis1DGame::updatePaddle(
    uint16_t& position, int8_t delta, uint16_t minimum, uint16_t maximum,
    int8_t& swingVelocity, uint32_t& lastDetentAt, uint32_t& lastMoveAt,
    uint32_t now) {
  if (delta == 0) {
    if (static_cast<uint32_t>(now - lastMoveAt) >
        Config::TENNIS_SWING_MEMORY_MS) {
      swingVelocity = 0;
    }
    return;
  }

  int16_t next = static_cast<int16_t>(position) +
                 static_cast<int16_t>(delta) *
                     Config::TENNIS_ENCODER_PIXELS_PER_DETENT;
  if (next < static_cast<int16_t>(minimum)) {
    next = minimum;
  } else if (next > static_cast<int16_t>(maximum)) {
    next = maximum;
  }
  position = static_cast<uint16_t>(next);

  const uint32_t interval = now - lastDetentAt;
  uint8_t speed = 1;
  if (magnitude(delta) > 1U ||
      (lastDetentAt != 0U && interval <= Config::TENNIS_FAST_DETENT_MS)) {
    speed = 3;
  } else if (lastDetentAt != 0U &&
             interval <= Config::TENNIS_MEDIUM_DETENT_MS) {
    speed = 2;
  }
  swingVelocity = delta > 0 ? static_cast<int8_t>(speed)
                            : -static_cast<int8_t>(speed);
  lastDetentAt = now;
  lastMoveAt = now;
}

void Tennis1DGame::updatePaddles(int8_t player1Delta, int8_t player2Delta,
                                 uint32_t now) {
  updatePaddle(player1Paddle_, player1Delta, HALF_PADDLE,
               Config::TENNIS_COURT_LENGTH - 1U - HALF_PADDLE,
               player1SwingVelocity_, player1LastDetentAt_,
               player1LastMoveAt_, now);
  updatePaddle(player2Paddle_, player2Delta,
               PLAYER_2_COURT_START + HALF_PADDLE,
               Config::LED_COUNT - 1U - HALF_PADDLE,
               player2SwingVelocity_, player2LastDetentAt_,
               player2LastMoveAt_, now);
}

void Tennis1DGame::launchShot(bool fromPlayer1, uint8_t power,
                              uint32_t now) {
  if (power < 1U) {
    power = 1U;
  } else if (power > 4U) {
    power = 4U;
  }

  const uint8_t depth = Config::TENNIS_LANDING_FRONT_MARGIN +
                        (power - 1U) * Config::TENNIS_LANDING_DEPTH_STEP;
  flightFrom_ = fromPlayer1 ? player1Paddle_ : player2Paddle_;
  flightTo_ = fromPlayer1 ? PLAYER_2_COURT_START + depth
                          : Config::TENNIS_COURT_LENGTH - 1U - depth;
  ballPosition_ = flightFrom_;
  bounceDirection_ = fromPlayer1 ? 1 : -1;
  shotPower_ = power;
  flightDurationMs_ = Config::gameplayInterval(
      Config::TENNIS_FLIGHT_BASE_MS -
      power * Config::TENNIS_FLIGHT_SPEEDUP_MS);
  phaseChangedAt_ = now;
  phase_ = Phase::Flying;
}

void Tennis1DGame::updateFlight(uint32_t now) {
  const uint32_t elapsed = now - phaseChangedAt_;
  if (elapsed >= flightDurationMs_) {
    ballPosition_ = flightTo_;
    lastBallStepAt_ = now;
    phase_ = Phase::Bouncing;
    return;
  }

  const int32_t distance =
      static_cast<int32_t>(flightTo_) - static_cast<int32_t>(flightFrom_);
  ballPosition_ = static_cast<uint16_t>(
      static_cast<int32_t>(flightFrom_) +
      distance * static_cast<int32_t>(elapsed) / flightDurationMs_);
}

bool Tennis1DGame::ballTouchesHitEdge(bool player1) const {
  if (player1) {
    const uint16_t edgeEnd = player1Paddle_ + HALF_PADDLE;
    const uint16_t edgeStart = edgeEnd - Config::TENNIS_HIT_EDGE_WIDTH + 1U;
    return ballPosition_ >= edgeStart && ballPosition_ <= edgeEnd;
  }

  const uint16_t edgeStart = player2Paddle_ - HALF_PADDLE;
  const uint16_t edgeEnd = edgeStart + Config::TENNIS_HIT_EDGE_WIDTH - 1U;
  return ballPosition_ >= edgeStart && ballPosition_ <= edgeEnd;
}

uint8_t Tennis1DGame::returnPower(bool player1) const {
  const int8_t velocity =
      player1 ? player1SwingVelocity_ : player2SwingVelocity_;
  const bool movingInward = player1 ? velocity > 0 : velocity < 0;
  if (!movingInward) {
    return 1U;
  }
  uint8_t power = 1U + magnitude(velocity);
  return power > 4U ? 4U : power;
}

bool Tennis1DGame::tryReturn(uint32_t now) {
  const bool player1 = bounceDirection_ < 0;
  if (!ballTouchesHitEdge(player1)) {
    return false;
  }

  const uint8_t power = returnPower(player1);
  lastHitAt_ = now;
  lastHitPosition_ = ballPosition_;
  lastHitPower_ = power;
  launchShot(player1, power, now);
  DEBUG_PRINT(player1 ? F("P1") : F("P2"));
  DEBUG_PRINT(F(" return power "));
  DEBUG_PRINTLN(power);
  return true;
}

void Tennis1DGame::updateBounce(uint32_t now) {
  if (tryReturn(now)) {
    return;
  }

  const uint8_t stepInterval = static_cast<uint8_t>(Config::gameplayInterval(
      Config::TENNIS_BOUNCE_BASE_STEP_MS -
      shotPower_ * Config::TENNIS_BOUNCE_SPEEDUP_MS));
  if (static_cast<uint32_t>(now - lastBallStepAt_) < stepInterval) {
    return;
  }
  lastBallStepAt_ += stepInterval;

  if (bounceDirection_ < 0) {
    if (ballPosition_ == 0U) {
      awardPoint(false, now);
    } else {
      --ballPosition_;
    }
  } else if (ballPosition_ >= Config::LED_COUNT - 1U) {
    awardPoint(true, now);
  } else {
    ++ballPosition_;
  }
}

void Tennis1DGame::awardPoint(bool player1Scored, uint32_t now) {
  if (player1Scored) {
    ++player1Score_;
  } else {
    ++player2Score_;
  }
  lastPointPlayer1_ = player1Scored;
  serveFromPlayer1_ = !serveFromPlayer1_;
  phaseChangedAt_ = now;
  phase_ = player1Score_ >= Config::TENNIS_WINNING_SCORE ||
                   player2Score_ >= Config::TENNIS_WINNING_SCORE
               ? Phase::GameOver
               : Phase::PointDelay;
}

void Tennis1DGame::update(uint32_t now) {
  const int8_t player1Delta = Controls::rotation(Controls::Player::One);
  const int8_t player2Delta = Controls::rotation(Controls::Player::Two);
  updatePaddles(player1Delta, player2Delta, now);

  if (phase_ == Phase::GameOver) {
    if (player1Delta != 0 || player2Delta != 0 ||
        Controls::primaryPressed(Controls::Player::One) ||
        Controls::primaryPressed(Controls::Player::Two)) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::ServeDelay) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::TENNIS_SERVE_DELAY_MS) {
      launchShot(serveFromPlayer1_, 2U, now);
    }
  } else if (phase_ == Phase::Flying) {
    updateFlight(now);
  } else if (phase_ == Phase::Bouncing) {
    updateBounce(now);
  } else if (static_cast<uint32_t>(now - phaseChangedAt_) >=
             Config::TENNIS_POINT_DELAY_MS) {
    phase_ = Phase::ServeDelay;
    phaseChangedAt_ = now;
  }
}

void Tennis1DGame::renderPaddle(uint16_t position, bool player1) const {
  const uint32_t fullColor = player1 ? Config::TENNIS_PLAYER_1_COLOR
                                     : Config::TENNIS_PLAYER_2_COLOR;
  const uint32_t dimColor = player1 ? 0x300000UL : 0x000030UL;
  const uint16_t start = position - HALF_PADDLE;
  for (uint8_t offset = 0; offset < Config::TENNIS_PADDLE_WIDTH; ++offset) {
    const bool hitEdge = player1
                             ? offset >= Config::TENNIS_PADDLE_WIDTH -
                                             Config::TENNIS_HIT_EDGE_WIDTH
                             : offset < Config::TENNIS_HIT_EDGE_WIDTH;
    LedManager::setStripPixel(start + offset,
                              hitEdge ? fullColor : dimColor);
  }
}

void Tennis1DGame::renderBall(uint32_t now) const {
  if (phase_ == Phase::Flying) {
    const uint32_t elapsed = now - phaseChangedAt_;
    const uint16_t progress = elapsed >= flightDurationMs_
                                  ? 255U
                                  : elapsed * 255U / flightDurationMs_;
    const uint16_t height =
        4UL * progress * (255U - progress) / 255U;
    const uint8_t radius = 1U + height / 52U;
    for (int8_t offset = -static_cast<int8_t>(radius);
         offset <= static_cast<int8_t>(radius); ++offset) {
      const int16_t pixel = static_cast<int16_t>(ballPosition_) + offset;
      if (pixel >= 0 && pixel < static_cast<int16_t>(Config::LED_COUNT)) {
        LedManager::setStripPixel(static_cast<uint16_t>(pixel),
                                  Config::TENNIS_ARC_COLOR);
      }
    }
    for (uint8_t offset = 0; offset < Config::TAPE_PIXEL_WIDTH; ++offset) {
      const int16_t pixel = static_cast<int16_t>(ballPosition_) + offset - 1;
      if (pixel >= 0 && pixel < static_cast<int16_t>(Config::LED_COUNT)) {
        LedManager::setStripPixel(static_cast<uint16_t>(pixel),
                                  Config::TENNIS_BALL_COLOR);
      }
    }
    return;
  }

  if (phase_ == Phase::Bouncing) {
    for (uint8_t offset = 0; offset < Config::TAPE_PIXEL_WIDTH; ++offset) {
      const int16_t pixel = static_cast<int16_t>(ballPosition_) + offset - 1;
      if (pixel >= 0 && pixel < static_cast<int16_t>(Config::LED_COUNT)) {
        LedManager::setStripPixel(static_cast<uint16_t>(pixel),
                                  Config::TENNIS_BALL_COLOR);
      }
    }
  }
}

void Tennis1DGame::renderHitEffect(uint32_t now) const {
  if (lastHitPower_ == 0U ||
      static_cast<uint32_t>(now - lastHitAt_) >= Config::TENNIS_HIT_EFFECT_MS) {
    return;
  }
  const uint8_t radius = Config::EXPLOSION_INTENSITY / 2U +
                         lastHitPower_ * Config::TAPE_PIXEL_WIDTH;
  const bool strobe = ((now - lastHitAt_) / 30U) % 2U == 0U;
  for (int8_t offset = -static_cast<int8_t>(radius);
       offset <= static_cast<int8_t>(radius); ++offset) {
    const int16_t pixel = static_cast<int16_t>(lastHitPosition_) + offset;
    if (pixel >= 0 && pixel < static_cast<int16_t>(Config::LED_COUNT) &&
        (strobe || (offset & 1) == 0)) {
      LedManager::setStripPixel(static_cast<uint16_t>(pixel),
                                Config::TENNIS_BALL_COLOR);
    }
  }
}

void Tennis1DGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::GameOver) {
    if ((now / 200U) % 2U == 0U) {
      const bool player1Won = player1Score_ > player2Score_;
      for (uint16_t pixel = 0; pixel < Config::LED_COUNT; ++pixel) {
        LedManager::setStripPixel(pixel, player1Won
                                            ? Config::TENNIS_PLAYER_1_COLOR
                                            : Config::TENNIS_PLAYER_2_COLOR);
      }
    }
    return;
  }

  renderPaddle(player1Paddle_, true);
  renderPaddle(player2Paddle_, false);

  if (phase_ == Phase::Flying) {
    for (int8_t offset = -1; offset <= 1; ++offset) {
      const int16_t pixel = static_cast<int16_t>(flightTo_) + offset;
      if (pixel >= 0 && pixel < static_cast<int16_t>(Config::LED_COUNT)) {
        LedManager::setStripPixel(static_cast<uint16_t>(pixel),
                                  Config::TENNIS_LANDING_MARKER_COLOR);
      }
    }
  } else if (phase_ == Phase::PointDelay && (now / 120U) % 2U == 0U) {
    const uint16_t start = lastPointPlayer1_ ? 0U : PLAYER_2_COURT_START;
    const uint32_t color = lastPointPlayer1_ ? Config::TENNIS_PLAYER_1_COLOR
                                             : Config::TENNIS_PLAYER_2_COLOR;
    for (uint8_t offset = 0; offset < Config::TENNIS_COURT_LENGTH; ++offset) {
      LedManager::setStripPixel(start + offset, color);
    }
  }

  renderBall(now);
  renderHitEffect(now);
}
