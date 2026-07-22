#pragma once

#include <Arduino.h>

class Firefighter1DGame {
 public:
  void start(uint32_t now);
  void update(uint32_t now);
  void render(uint32_t now) const;
  uint16_t score() const { return score_; }

 private:
  struct Fire {
    uint32_t ignitedAt = 0;
    uint16_t lifetime = 0;
    uint8_t cell = 0;
    bool active = false;
  };

  enum class Phase : uint8_t {
    Playing,
    Error,
    GameOver,
  };

  uint16_t nextRandom();
  uint16_t burnDuration() const;
  uint8_t desiredFireCount() const;
  void movePlayer(int8_t delta);
  void clearFires();
  void fillFires(uint32_t now);
  bool spawnFire(uint32_t now);
  void spray(uint32_t now);
  void loseLife(uint32_t now);

  Fire fires_[3];
  Phase phase_ = Phase::Playing;
  uint32_t phaseChangedAt_ = 0;
  uint32_t sprayStartedAt_ = 0;
  uint16_t randomState_ = 1;
  uint16_t score_ = 0;
  uint8_t playerCell_ = 0;
  uint8_t sprayOriginCell_ = 0;
  uint8_t sprayTargetCell_ = 0;
  uint8_t lives_ = 0;
  bool sprayVisible_ = false;
  bool sprayHit_ = false;
};
