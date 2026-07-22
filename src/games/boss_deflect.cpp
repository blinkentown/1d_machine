#include "games/boss_deflect.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint16_t GATE_START =
    Config::BOSS_DEFLECT_GATE_CENTER - Config::BOSS_DEFLECT_GATE_WIDTH / 2U;
constexpr uint16_t GATE_END = GATE_START + Config::BOSS_DEFLECT_GATE_WIDTH - 1U;
constexpr uint16_t BOSS_START =
    Config::LED_COUNT - Config::BOSS_DEFLECT_BOSS_WIDTH;

}  // namespace

void BossDeflectGame::start(uint32_t now) {
  score_ = 0;
  lives_ = Config::BOSS_DEFLECT_STARTING_LIVES;
  level_ = 1;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(F("Boss Deflect started: match attacks inside the gate"));
  startLevel(now);
}

uint16_t BossDeflectGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

uint32_t BossDeflectGame::colorFor(uint8_t color) {
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

int8_t BossDeflectGame::pressedColor() {
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

uint8_t BossDeflectGame::healthForLevel() const {
  const uint16_t health = Config::BOSS_DEFLECT_BASE_HEALTH + level_ - 1U;
  return health > Config::BOSS_DEFLECT_MAX_HEALTH
             ? Config::BOSS_DEFLECT_MAX_HEALTH
             : static_cast<uint8_t>(health);
}

uint16_t BossDeflectGame::incomingStepInterval() const {
  const uint8_t levelSpeedup =
      level_ > Config::BOSS_DEFLECT_MAX_LEVEL_SPEEDUP
          ? Config::BOSS_DEFLECT_MAX_LEVEL_SPEEDUP
          : level_ - 1U;
  return Config::gameplayInterval(
      Config::BOSS_DEFLECT_INITIAL_STEP_MS - levelSpeedup);
}

void BossDeflectGame::startLevel(uint32_t now) {
  bossHealth_ = healthForLevel();
  spawnAttack(now);
}

void BossDeflectGame::spawnAttack(uint32_t now) {
  attackPosition_ = BOSS_START - 1U;
  attackColor_ = static_cast<uint8_t>(nextRandom() & 0x03U);
  lastStepAt_ = now;
  phaseChangedAt_ = now;
  phase_ = Phase::Incoming;
}

bool BossDeflectGame::attackInGate() const {
  return attackPosition_ >= GATE_START && attackPosition_ <= GATE_END;
}

void BossDeflectGame::failDefense(uint32_t now) {
  if (lives_ > 0) {
    --lives_;
  }
  feedbackSuccess_ = false;
  phaseChangedAt_ = now;
  phase_ = lives_ == 0 ? Phase::GameOver : Phase::Feedback;
}

void BossDeflectGame::hitBoss(uint32_t now) {
  if (bossHealth_ > 0) {
    --bossHealth_;
  }
  if (score_ < 999U) {
    ++score_;
  }
  feedbackSuccess_ = true;
  phaseChangedAt_ = now;
  phase_ = bossHealth_ == 0 ? Phase::LevelClear : Phase::Feedback;
}

void BossDeflectGame::moveIncoming(uint32_t now) {
  const uint16_t interval = incomingStepInterval();
  uint8_t steps = static_cast<uint8_t>((now - lastStepAt_) / interval);
  if (steps > Config::BOSS_DEFLECT_MAX_CATCHUP_STEPS) {
    steps = Config::BOSS_DEFLECT_MAX_CATCHUP_STEPS;
  }
  if (steps == 0) {
    return;
  }
  lastStepAt_ += static_cast<uint32_t>(steps) * interval;
  attackPosition_ = steps >= attackPosition_ ? 0U : attackPosition_ - steps;
  if (attackPosition_ < GATE_START) {
    failDefense(now);
  }
}

void BossDeflectGame::moveReflected(uint32_t now) {
  const uint16_t interval =
      Config::gameplayInterval(Config::BOSS_DEFLECT_REFLECTED_STEP_MS);
  uint8_t steps = static_cast<uint8_t>((now - lastStepAt_) / interval);
  if (steps > Config::BOSS_DEFLECT_MAX_CATCHUP_STEPS) {
    steps = Config::BOSS_DEFLECT_MAX_CATCHUP_STEPS;
  }
  if (steps == 0) {
    return;
  }
  lastStepAt_ += static_cast<uint32_t>(steps) * interval;
  if (attackPosition_ + steps >= BOSS_START) {
    attackPosition_ = BOSS_START;
    hitBoss(now);
  } else {
    attackPosition_ += steps;
  }
}

void BossDeflectGame::update(uint32_t now) {
  const int8_t pressed = pressedColor();
  if (phase_ == Phase::GameOver) {
    if (pressed >= 0) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::LevelClear) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::BOSS_DEFLECT_LEVEL_CLEAR_MS) {
      if (level_ < 255U) {
        ++level_;
      }
      if (lives_ < Config::BOSS_DEFLECT_STARTING_LIVES) {
        ++lives_;
      }
      startLevel(now);
    }
    return;
  }

  if (phase_ == Phase::Feedback) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::BOSS_DEFLECT_FEEDBACK_MS) {
      spawnAttack(now);
    }
    return;
  }

  if (phase_ == Phase::Reflected) {
    moveReflected(now);
    return;
  }

  if (pressed >= 0) {
    if (attackInGate() && static_cast<uint8_t>(pressed) == attackColor_) {
      phase_ = Phase::Reflected;
      lastStepAt_ = now;
    } else {
      failDefense(now);
    }
    return;
  }
  moveIncoming(now);
}

void BossDeflectGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::LevelClear) {
    const uint16_t count = static_cast<uint16_t>(
        (static_cast<uint32_t>(now - phaseChangedAt_) * Config::LED_COUNT) /
        Config::BOSS_DEFLECT_LEVEL_CLEAR_MS);
    LedManager::setStripRange(0, count, Config::BOSS_DEFLECT_SUCCESS_COLOR);
    return;
  }

  uint32_t gateColor = Config::BOSS_DEFLECT_GATE_COLOR;
  if (phase_ == Phase::Feedback && !feedbackSuccess_) {
    gateColor = Config::BOSS_DEFLECT_ERROR_COLOR;
  } else if (phase_ == Phase::GameOver) {
    gateColor = ((now / 160U) & 1U) == 0U
                    ? Config::BOSS_DEFLECT_ERROR_COLOR
                    : 0;
  }
  LedManager::setStripRange(GATE_START, Config::BOSS_DEFLECT_GATE_WIDTH,
                            gateColor);

  const uint32_t bossColor =
      phase_ == Phase::Feedback && feedbackSuccess_
          ? (((now / 45U) & 1U) == 0U ? 0xFFFFFFUL
                                      : Config::BOSS_DEFLECT_BOSS_COLOR)
          : Config::BOSS_DEFLECT_BOSS_COLOR;
  LedManager::setStripRange(BOSS_START, Config::BOSS_DEFLECT_BOSS_WIDTH,
                            0x300000UL);
  const uint8_t maximumHealth = healthForLevel();
  const uint8_t brightWidth = static_cast<uint8_t>(
      (static_cast<uint16_t>(bossHealth_) * Config::BOSS_DEFLECT_BOSS_WIDTH) /
      maximumHealth);
  LedManager::setStripRange(Config::LED_COUNT - brightWidth, brightWidth,
                            bossColor);

  if (phase_ == Phase::Incoming || phase_ == Phase::Reflected) {
    const uint32_t attackColor = phase_ == Phase::Reflected
                                     ? 0xFFFFFFUL
                                     : colorFor(attackColor_);
    for (uint8_t width = 0; width < Config::BOSS_DEFLECT_ATTACK_WIDTH;
         ++width) {
      const int16_t pixel = phase_ == Phase::Incoming
                                ? static_cast<int16_t>(attackPosition_) + width
                                : static_cast<int16_t>(attackPosition_) - width;
      if (pixel >= 0 && pixel < static_cast<int16_t>(Config::LED_COUNT)) {
        LedManager::setStripPixel(static_cast<uint16_t>(pixel), attackColor);
      }
    }
  }

  for (uint8_t life = 0; life < lives_; ++life) {
    LedManager::setStripPixel(life, Config::BOSS_DEFLECT_LIFE_COLOR);
  }
}
