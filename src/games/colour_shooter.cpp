#include "games/colour_shooter.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t TARGET_TRAIL_LENGTH = 2;
constexpr uint8_t SHOT_TRAIL_LENGTH = 2;

}  // namespace

void ColourShooterGame::start(uint32_t now) {
  score_ = 0;
  lives_ = Config::COLOUR_SHOOTER_STARTING_LIVES;
  stepIntervalMs_ = Config::COLOUR_SHOOTER_INITIAL_STEP_MS;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }

  resetObjects();
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_TARGET_COUNT;
       ++index) {
    const uint16_t position =
        Config::COLOUR_SHOOTER_FIRST_TARGET_POSITION +
        static_cast<uint16_t>(index) * Config::COLOUR_SHOOTER_TARGET_SPACING;
    spawnTarget(position);
  }

  lastTargetStepAt_ = now;
  lastSpawnAt_ = now;
  phase_ = Phase::Playing;

  Serial.println(F("Colour Shooter: Incoming Wave"));
  Serial.println(F("Fire matching R/G/B/Y pixels into the incoming row"));
}

void ColourShooterGame::resetObjects() {
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_TARGET_COUNT;
       ++index) {
    targets_[index].active = false;
  }
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_MAX_SHOTS; ++index) {
    shots_[index].active = false;
  }
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_MAX_DISSOLVES;
       ++index) {
    dissolves_[index].active = false;
  }
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

uint32_t ColourShooterGame::scaleColor(uint32_t color, uint8_t scale) {
  const uint8_t red = static_cast<uint8_t>((color >> 16) & 0xFFU);
  const uint8_t green = static_cast<uint8_t>((color >> 8) & 0xFFU);
  const uint8_t blue = static_cast<uint8_t>(color & 0xFFU);
  const uint8_t scaledRed =
      static_cast<uint8_t>((static_cast<uint16_t>(red) * scale) / 255U);
  const uint8_t scaledGreen =
      static_cast<uint8_t>((static_cast<uint16_t>(green) * scale) / 255U);
  const uint8_t scaledBlue =
      static_cast<uint8_t>((static_cast<uint16_t>(blue) * scale) / 255U);
  return (static_cast<uint32_t>(scaledRed) << 16) |
         (static_cast<uint32_t>(scaledGreen) << 8) | scaledBlue;
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

bool ColourShooterGame::spawnTarget(uint16_t position) {
  if (position >= Config::LED_COUNT) {
    return false;
  }

  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_TARGET_COUNT;
       ++index) {
    if (!targets_[index].active) {
      targets_[index].position = position;
      targets_[index].colorIndex = nextColorIndex();
      targets_[index].active = true;
      return true;
    }
  }
  return false;
}

bool ColourShooterGame::farEndIsClear() const {
  const uint16_t minimumPosition =
      Config::LED_COUNT - Config::COLOUR_SHOOTER_TARGET_SPACING;
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_TARGET_COUNT;
       ++index) {
    if (targets_[index].active &&
        targets_[index].position >= minimumPosition) {
      return false;
    }
  }
  return true;
}

void ColourShooterGame::spawnTargetAtFarEnd(uint32_t now) {
  if (static_cast<uint32_t>(now - lastSpawnAt_) <
          Config::COLOUR_SHOOTER_SPAWN_INTERVAL_MS ||
      !farEndIsClear()) {
    return;
  }

  if (spawnTarget(Config::LED_COUNT - 1)) {
    lastSpawnAt_ = now;
  }
}

void ColourShooterGame::launchShot(uint8_t colorIndex, uint32_t now) {
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_MAX_SHOTS; ++index) {
    if (!shots_[index].active) {
      shots_[index].position = Config::COLOUR_SHOOTER_HOME_POSITION + 1;
      shots_[index].lastStepAt = now;
      shots_[index].colorIndex = colorIndex;
      shots_[index].active = true;
      return;
    }
  }
}

void ColourShooterGame::handleButtonPress(uint32_t now) {
  if (InputManager::wasPressed(InputManager::Button::Game1)) {
    launchShot(0, now);
  }
  if (InputManager::wasPressed(InputManager::Button::Game2)) {
    launchShot(1, now);
  }
  if (InputManager::wasPressed(InputManager::Button::Game3)) {
    launchShot(2, now);
  }
  if (InputManager::wasPressed(InputManager::Button::Game4)) {
    launchShot(3, now);
  }
}

void ColourShooterGame::startDissolve(uint16_t position, uint32_t color,
                                      uint32_t now) {
  uint8_t slot = 0;
  uint32_t oldestStartedAt = 0xFFFFFFFFUL;
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_MAX_DISSOLVES;
       ++index) {
    if (!dissolves_[index].active) {
      slot = index;
      oldestStartedAt = 0;
      break;
    }
    if (dissolves_[index].startedAt < oldestStartedAt) {
      oldestStartedAt = dissolves_[index].startedAt;
      slot = index;
    }
  }

  dissolves_[slot].position = position;
  dissolves_[slot].color = color;
  dissolves_[slot].startedAt = now;
  dissolves_[slot].active = true;
}

bool ColourShooterGame::resolveShotCollision(Shot& shot, uint32_t now) {
  int8_t hitIndex = -1;
  uint16_t hitPosition = 0;
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_TARGET_COUNT;
       ++index) {
    if (targets_[index].active &&
        targets_[index].position <= shot.position &&
        (hitIndex < 0 || targets_[index].position > hitPosition)) {
      hitIndex = static_cast<int8_t>(index);
      hitPosition = targets_[index].position;
    }
  }

  if (hitIndex < 0) {
    return false;
  }

  Target& target = targets_[static_cast<uint8_t>(hitIndex)];
  shot.active = false;
  if (shot.colorIndex == target.colorIndex) {
    const uint32_t color = colorForIndex(target.colorIndex);
    target.active = false;
    ++score_;
    startDissolve(hitPosition, color, now);

    if (score_ % Config::COLOUR_SHOOTER_SPEEDUP_EVERY == 0 &&
        stepIntervalMs_ > Config::COLOUR_SHOOTER_MINIMUM_STEP_MS) {
      const uint16_t faster = stepIntervalMs_ -
                              Config::COLOUR_SHOOTER_SPEEDUP_MS;
      stepIntervalMs_ =
          faster < Config::COLOUR_SHOOTER_MINIMUM_STEP_MS
              ? Config::COLOUR_SHOOTER_MINIMUM_STEP_MS
              : faster;
    }

    Serial.print(F("Dissolved. Score: "));
    Serial.println(score_);
  } else {
    startDissolve(hitPosition, Config::COLOUR_SHOOTER_ERROR_COLOR, now);
    Serial.println(F("Wrong shot blocked"));
  }
  return true;
}

void ColourShooterGame::updateShots(uint32_t now) {
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_MAX_SHOTS; ++index) {
    Shot& shot = shots_[index];
    if (!shot.active) {
      continue;
    }

    uint8_t steps = static_cast<uint8_t>(
        (now - shot.lastStepAt) / Config::COLOUR_SHOOTER_SHOT_STEP_MS);
    if (steps > 8) {
      steps = 8;
    }
    if (steps == 0) {
      continue;
    }
    shot.lastStepAt +=
        static_cast<uint32_t>(steps) * Config::COLOUR_SHOOTER_SHOT_STEP_MS;

    while (steps-- > 0 && shot.active) {
      if (shot.position >= Config::LED_COUNT - 1) {
        shot.active = false;
        break;
      }
      ++shot.position;
      resolveShotCollision(shot, now);
    }
  }
}

void ColourShooterGame::loseLife(uint32_t now,
                                 const __FlashStringHelper* reason) {
  if (lives_ > 0) {
    --lives_;
  }
  startDissolve(Config::COLOUR_SHOOTER_HOME_POSITION,
                Config::COLOUR_SHOOTER_ERROR_COLOR, now);

  Serial.print(reason);
  Serial.print(F(". Lives: "));
  Serial.println(lives_);

  if (lives_ == 0) {
    phase_ = Phase::GameOver;
    Serial.print(F("Game over. Score: "));
    Serial.println(score_);
    Serial.println(F("Press any color button to restart"));
  }
}

void ColourShooterGame::updateTargets(uint32_t now) {
  uint8_t steps =
      static_cast<uint8_t>((now - lastTargetStepAt_) / stepIntervalMs_);
  if (steps > 4) {
    steps = 4;
  }
  if (steps == 0) {
    return;
  }
  lastTargetStepAt_ += static_cast<uint32_t>(steps) * stepIntervalMs_;

  while (steps-- > 0 && phase_ == Phase::Playing) {
    for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_TARGET_COUNT;
         ++index) {
      Target& target = targets_[index];
      if (!target.active) {
        continue;
      }
      if (target.position <= Config::COLOUR_SHOOTER_HOME_POSITION + 1) {
        target.active = false;
        loseLife(now, F("Incoming pixel reached home"));
        if (phase_ == Phase::GameOver) {
          break;
        }
      } else {
        --target.position;
      }
    }
  }
}

void ColourShooterGame::updateDissolves(uint32_t now) {
  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_MAX_DISSOLVES;
       ++index) {
    if (dissolves_[index].active &&
        static_cast<uint32_t>(now - dissolves_[index].startedAt) >=
            Config::COLOUR_SHOOTER_DISSOLVE_MS) {
      dissolves_[index].active = false;
    }
  }
}

void ColourShooterGame::update(uint32_t now) {
  updateDissolves(now);

  if (phase_ == Phase::GameOver) {
    if (pressedColorIndex() >= 0) {
      start(now);
    }
    return;
  }

  handleButtonPress(now);
  updateShots(now);
  updateTargets(now);
  if (phase_ == Phase::Playing) {
    spawnTargetAtFarEnd(now);
  }
}

void ColourShooterGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::GameOver) {
    if ((now / 300U) % 2U == 0U) {
      LedManager::setStripPixel(Config::LED_COUNT / 2 - 1,
                                Config::COLOUR_SHOOTER_ERROR_COLOR);
      LedManager::setStripPixel(Config::LED_COUNT / 2,
                                Config::COLOUR_SHOOTER_ERROR_COLOR);
      LedManager::setStripPixel(Config::LED_COUNT / 2 + 1,
                                Config::COLOUR_SHOOTER_ERROR_COLOR);
    }
    return;
  }

  for (uint8_t index = 0; index < lives_; ++index) {
    LedManager::setStripPixel(index, scaleColor(Config::BUTTON_2_COLOR, 96));
  }
  LedManager::setStripPixel(Config::COLOUR_SHOOTER_HOME_POSITION, 0x404040UL);

  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_TARGET_COUNT;
       ++index) {
    const Target& target = targets_[index];
    if (!target.active) {
      continue;
    }
    const uint32_t color = colorForIndex(target.colorIndex);
    LedManager::setStripPixel(target.position, color);
    for (uint8_t trail = 1; trail <= TARGET_TRAIL_LENGTH; ++trail) {
      const uint16_t trailPosition = target.position + trail;
      if (trailPosition < Config::LED_COUNT) {
        LedManager::setStripPixel(
            trailPosition,
            scaleColor(color, trail == 1 ? 96 : 32));
      }
    }
  }

  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_MAX_SHOTS; ++index) {
    const Shot& shot = shots_[index];
    if (!shot.active) {
      continue;
    }
    const uint32_t color = colorForIndex(shot.colorIndex);
    LedManager::setStripPixel(shot.position, color);
    for (uint8_t trail = 1; trail <= SHOT_TRAIL_LENGTH; ++trail) {
      if (shot.position >= trail) {
        LedManager::setStripPixel(
            shot.position - trail,
            scaleColor(color, trail == 1 ? 128 : 48));
      }
    }
  }

  for (uint8_t index = 0; index < Config::COLOUR_SHOOTER_MAX_DISSOLVES;
       ++index) {
    const Dissolve& dissolve = dissolves_[index];
    if (!dissolve.active) {
      continue;
    }
    const uint32_t elapsed = now - dissolve.startedAt;
    const uint8_t radius = static_cast<uint8_t>(
        1U + (elapsed * 7U) / Config::COLOUR_SHOOTER_DISSOLVE_MS);
    const uint8_t brightness = static_cast<uint8_t>(
        255U - (elapsed * 220U) / Config::COLOUR_SHOOTER_DISSOLVE_MS);
    const uint32_t faded = scaleColor(dissolve.color, brightness);

    LedManager::setStripPixel(dissolve.position,
                              scaleColor(dissolve.color, brightness / 3U));
    if (dissolve.position >= radius) {
      LedManager::setStripPixel(dissolve.position - radius, faded);
    }
    if (dissolve.position + radius < Config::LED_COUNT) {
      LedManager::setStripPixel(dissolve.position + radius, faded);
    }
  }
}
