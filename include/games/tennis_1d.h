#pragma once

#include <Arduino.h>

class Tennis1DGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint8_t player1Score() const { return player1Score_; }
  uint8_t player2Score() const { return player2Score_; }

 private:
  enum class Phase : uint8_t {
    ServeDelay,
    Flying,
    Bouncing,
    PointDelay,
    GameOver,
  };

  void resetPaddles();
  void updatePaddles(int8_t player1Delta, int8_t player2Delta,
                     uint32_t now);
  void updatePaddle(uint16_t& position, int8_t delta, uint16_t minimum,
                    uint16_t maximum, int8_t& swingVelocity,
                    uint32_t& lastDetentAt, uint32_t& lastMoveAt,
                    uint32_t now);
  void launchShot(bool fromPlayer1, uint8_t power, uint32_t now);
  void updateFlight(uint32_t now);
  void updateBounce(uint32_t now);
  bool tryReturn(uint32_t now);
  bool ballTouchesHitEdge(bool player1) const;
  uint8_t returnPower(bool player1) const;
  void awardPoint(bool player1Scored, uint32_t now);
  void renderPaddle(uint16_t position, bool player1) const;
  void renderBall(uint32_t now) const;
  void renderHitEffect(uint32_t now) const;

  Phase phase_ = Phase::ServeDelay;
  uint16_t player1Paddle_ = 0;
  uint16_t player2Paddle_ = 0;
  uint16_t ballPosition_ = 0;
  uint16_t flightFrom_ = 0;
  uint16_t flightTo_ = 0;
  uint16_t flightDurationMs_ = 0;
  uint32_t phaseChangedAt_ = 0;
  uint32_t lastBallStepAt_ = 0;
  uint32_t player1LastDetentAt_ = 0;
  uint32_t player2LastDetentAt_ = 0;
  uint32_t player1LastMoveAt_ = 0;
  uint32_t player2LastMoveAt_ = 0;
  uint32_t lastHitAt_ = 0;
  uint16_t lastHitPosition_ = 0;
  int8_t player1SwingVelocity_ = 0;
  int8_t player2SwingVelocity_ = 0;
  int8_t bounceDirection_ = 1;
  uint8_t shotPower_ = 1;
  uint8_t lastHitPower_ = 0;
  uint8_t player1Score_ = 0;
  uint8_t player2Score_ = 0;
  bool serveFromPlayer1_ = true;
  bool lastPointPlayer1_ = true;
};
