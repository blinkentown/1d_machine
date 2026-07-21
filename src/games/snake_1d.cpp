#include "games/snake_1d.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

static_assert(Config::GAME_PIXEL_WIDTH > 0,
              "Game pixel width must be at least one LED");
static_assert(Config::SNAKE_SPECIAL_WIDTH_CELLS ==
                  Config::SNAKE_SPECIAL_HITS,
              "Each armor hit removes one logical cell");

void Snake1DGame::start(uint32_t now) {
  score_ = 0;
  combo_ = 0;
  lives_ = Config::SNAKE_STARTING_LIVES;
  baseStepIntervalMs_ = Config::gameplayInterval(
      Config::SNAKE_INITIAL_STEP_MS, Config::SNAKE_SPEED_PERCENT);
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }

  resetObjects();
  createInitialSnake();
  lastSnakeStepAt_ = now;
  phase_ = Phase::Playing;

  Serial.println(F("Snake 1D started"));
  Serial.println(F("Shoot the matching color at the continuous snake head"));
  Serial.println(F("Rainbow bonus accepts any three color shots"));
}

void Snake1DGame::resetObjects() {
  segmentCount_ = 0;
  for (uint8_t index = 0; index < Config::SNAKE_MAX_SHOTS; ++index) {
    shots_[index].active = false;
  }
  for (uint8_t index = 0; index < Config::SNAKE_MAX_BLASTS; ++index) {
    blasts_[index].active = false;
  }
}

uint8_t Snake1DGame::nextColorIndex() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return static_cast<uint8_t>(randomState_ & 0x03U);
}

uint32_t Snake1DGame::colorForIndex(uint8_t index) {
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

uint32_t Snake1DGame::rainbowColor(uint8_t index) {
  switch (index % 6U) {
    case 0:
      return 0xFF0000UL;
    case 1:
      return 0xFFFF00UL;
    case 2:
      return 0x00FF00UL;
    case 3:
      return 0x00FFFFUL;
    case 4:
      return 0x0000FFUL;
    default:
      return 0xFF00FFUL;
  }
}

uint32_t Snake1DGame::scaleColor(uint32_t color, uint8_t scale) {
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

int8_t Snake1DGame::pressedColorIndex() {
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

uint8_t Snake1DGame::segmentWidthCells(const Segment& segment) const {
  return segment.special ? segment.hitPoints : 1U;
}

uint16_t Snake1DGame::totalWidthPixels() const {
  uint16_t width = 0;
  for (uint8_t index = 0; index < segmentCount_; ++index) {
    width += static_cast<uint16_t>(segmentWidthCells(segments_[index])) *
             Config::GAME_PIXEL_WIDTH;
  }
  return width;
}

bool Snake1DGame::appendSegment(bool special) {
  if (segmentCount_ >= Config::SNAKE_MAX_SEGMENTS) {
    return false;
  }

  uint8_t colorIndex = nextColorIndex();
  if (!special && segmentCount_ > 0) {
    const Segment& previous = segments_[segmentCount_ - 1];
    if (!previous.special && colorIndex == previous.colorIndex) {
      colorIndex = static_cast<uint8_t>(
          (colorIndex + 1U + (randomState_ & 0x01U)) & 0x03U);
    }
  }

  Segment& segment = segments_[segmentCount_++];
  segment.special = special;
  segment.hitPoints = special ? Config::SNAKE_SPECIAL_HITS : 1U;
  segment.colorIndex = colorIndex;
  return true;
}

void Snake1DGame::createInitialSnake() {
  uint8_t cells = 0;
  bool specialAdded = false;
  while (cells < Config::SNAKE_INITIAL_LENGTH_CELLS &&
         segmentCount_ < Config::SNAKE_MAX_SEGMENTS) {
    bool special = false;
    if (!specialAdded && cells >= Config::SNAKE_INITIAL_LENGTH_CELLS / 3U) {
      special = true;
    } else if (cells > 0 &&
               (randomState_ % Config::SNAKE_SPECIAL_CHANCE) == 0U) {
      special = true;
    }

    const uint8_t width =
        special ? Config::SNAKE_SPECIAL_WIDTH_CELLS : 1U;
    if (cells + width > Config::SNAKE_INITIAL_LENGTH_CELLS) {
      special = false;
    }
    appendSegment(special);
    cells += special ? Config::SNAKE_SPECIAL_WIDTH_CELLS : 1U;
    specialAdded = specialAdded || special;
    nextColorIndex();
  }

  headPosition_ = Config::LED_COUNT - totalWidthPixels();
}

void Snake1DGame::appendAtTail() {
  if (segmentCount_ == 0) {
    appendSegment(false);
    headPosition_ = Config::LED_COUNT - Config::GAME_PIXEL_WIDTH;
    return;
  }

  const uint16_t tailEnd = headPosition_ + totalWidthPixels();
  bool special =
      (randomState_ % Config::SNAKE_SPECIAL_CHANCE) == 0U;
  uint16_t newWidth =
      static_cast<uint16_t>(special ? Config::SNAKE_SPECIAL_WIDTH_CELLS : 1U) *
      Config::GAME_PIXEL_WIDTH;
  if (tailEnd + newWidth > Config::LED_COUNT) {
    special = false;
    newWidth = Config::GAME_PIXEL_WIDTH;
  }
  if (tailEnd + newWidth <= Config::LED_COUNT && appendSegment(special)) {
    nextColorIndex();
  }
}

void Snake1DGame::removeFrontSegment() {
  if (segmentCount_ == 0) {
    return;
  }
  for (uint8_t index = 1; index < segmentCount_; ++index) {
    segments_[index - 1] = segments_[index];
  }
  --segmentCount_;
}

void Snake1DGame::trimTailToStrip() {
  while (segmentCount_ > 1 &&
         headPosition_ + totalWidthPixels() > Config::LED_COUNT) {
    --segmentCount_;
  }
}

void Snake1DGame::launchShot(uint8_t colorIndex, uint32_t now) {
  for (uint8_t index = 0; index < Config::SNAKE_MAX_SHOTS; ++index) {
    if (!shots_[index].active) {
      shots_[index].position =
          Config::SNAKE_HOME_POSITION + Config::GAME_PIXEL_WIDTH;
      shots_[index].lastStepAt = now;
      shots_[index].colorIndex = colorIndex;
      shots_[index].active = true;
      return;
    }
  }
}

void Snake1DGame::handleButtonPress(uint32_t now) {
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

void Snake1DGame::startBlast(uint16_t position, uint32_t color,
                             uint8_t intensity, uint32_t now) {
  uint8_t slot = 0;
  uint32_t oldest = 0xFFFFFFFFUL;
  for (uint8_t index = 0; index < Config::SNAKE_MAX_BLASTS; ++index) {
    if (!blasts_[index].active) {
      slot = index;
      oldest = 0;
      break;
    }
    if (blasts_[index].startedAt < oldest) {
      oldest = blasts_[index].startedAt;
      slot = index;
    }
  }

  blasts_[slot].position = position;
  blasts_[slot].color = color;
  blasts_[slot].intensity = intensity;
  blasts_[slot].startedAt = now;
  blasts_[slot].active = true;
}

bool Snake1DGame::resolveShotCollision(Shot& shot, uint32_t now) {
  if (segmentCount_ == 0 || shot.position < headPosition_) {
    return false;
  }

  Segment& head = segments_[0];
  shot.active = false;
  if (!head.special && shot.colorIndex != head.colorIndex) {
    combo_ = 0;
    startBlast(headPosition_, Config::SNAKE_ERROR_COLOR,
               Config::EXPLOSION_INTENSITY, now);
    Serial.println(F("Wrong color blocked by snake head"));
    return true;
  }

  ++score_;
  if (combo_ < 255) {
    ++combo_;
  }
  const uint8_t comboBoost = combo_ / 4U > 3U ? 3U : combo_ / 4U;
  const uint32_t impactColor =
      head.special ? colorForIndex(shot.colorIndex)
                   : colorForIndex(head.colorIndex);
  startBlast(headPosition_, impactColor,
             Config::EXPLOSION_INTENSITY + comboBoost, now);

  headPosition_ += Config::GAME_PIXEL_WIDTH;
  if (head.special) {
    --head.hitPoints;
    if (head.hitPoints == 0) {
      removeFrontSegment();
      Serial.println(F("Rainbow armor destroyed"));
    } else {
      Serial.print(F("Rainbow bonus hit. Remaining: "));
      Serial.println(head.hitPoints);
    }
  } else {
    removeFrontSegment();
  }

  if (score_ % Config::SNAKE_SPEEDUP_EVERY == 0 &&
      baseStepIntervalMs_ > Config::gameplayInterval(
                                Config::SNAKE_MINIMUM_STEP_MS,
                                Config::SNAKE_SPEED_PERCENT)) {
    const uint16_t faster = baseStepIntervalMs_ - Config::SNAKE_SPEEDUP_MS;
    const uint16_t minimum = Config::gameplayInterval(
        Config::SNAKE_MINIMUM_STEP_MS, Config::SNAKE_SPEED_PERCENT);
    baseStepIntervalMs_ = faster < minimum ? minimum : faster;
  }

  Serial.print(F("Snake hit. Score: "));
  Serial.print(score_);
  Serial.print(F(" Combo: "));
  Serial.println(combo_);
  return true;
}

void Snake1DGame::updateShots(uint32_t now) {
  for (uint8_t index = 0; index < Config::SNAKE_MAX_SHOTS; ++index) {
    Shot& shot = shots_[index];
    if (!shot.active) {
      continue;
    }

    const uint16_t shotInterval =
        Config::gameplayInterval(Config::SNAKE_SHOT_STEP_MS);
    uint8_t steps =
        static_cast<uint8_t>((now - shot.lastStepAt) / shotInterval);
    if (steps > 8) {
      steps = 8;
    }
    if (steps == 0) {
      continue;
    }
    shot.lastStepAt += static_cast<uint32_t>(steps) * shotInterval;

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

uint16_t Snake1DGame::effectiveStepInterval() const {
  const uint16_t slowZone =
      static_cast<uint16_t>(Config::SNAKE_SLOW_ZONE_CELLS) *
      Config::GAME_PIXEL_WIDTH;
  if (headPosition_ >= slowZone) {
    return baseStepIntervalMs_;
  }

  const uint16_t proximity = slowZone - headPosition_;
  const uint32_t slowdown =
      static_cast<uint32_t>(baseStepIntervalMs_) *
      Config::SNAKE_CLOSE_SLOWDOWN_PERCENT * proximity /
      (static_cast<uint32_t>(slowZone) * 100U);
  return baseStepIntervalMs_ + static_cast<uint16_t>(slowdown);
}

void Snake1DGame::handleBreach(uint32_t now) {
  if (lives_ > 0) {
    --lives_;
  }
  combo_ = 0;
  startBlast(Config::SNAKE_HOME_POSITION, Config::SNAKE_ERROR_COLOR,
             Config::EXPLOSION_INTENSITY + 2U, now);

  Serial.print(F("Snake breached. Lives: "));
  Serial.println(lives_);
  if (lives_ == 0) {
    phase_ = Phase::GameOver;
    Serial.print(F("Snake game over. Score: "));
    Serial.println(score_);
    Serial.println(F("Press any color button to restart"));
    return;
  }

  headPosition_ +=
      static_cast<uint16_t>(Config::SNAKE_BREACH_PUSHBACK_CELLS) *
      Config::GAME_PIXEL_WIDTH;
  trimTailToStrip();
}

void Snake1DGame::updateSnake(uint32_t now) {
  const uint16_t interval = effectiveStepInterval();
  if (static_cast<uint32_t>(now - lastSnakeStepAt_) < interval) {
    return;
  }
  lastSnakeStepAt_ = now;

  if (headPosition_ <= Config::SNAKE_HOME_POSITION + 1U) {
    handleBreach(now);
  } else {
    --headPosition_;
    appendAtTail();
  }
}

void Snake1DGame::updateBlasts(uint32_t now) {
  for (uint8_t index = 0; index < Config::SNAKE_MAX_BLASTS; ++index) {
    if (blasts_[index].active &&
        static_cast<uint32_t>(now - blasts_[index].startedAt) >=
            Config::SNAKE_BLAST_MS) {
      blasts_[index].active = false;
    }
  }
}

void Snake1DGame::update(uint32_t now) {
  updateBlasts(now);
  if (phase_ == Phase::GameOver) {
    if (pressedColorIndex() >= 0) {
      start(now);
    }
    return;
  }

  handleButtonPress(now);
  updateShots(now);
  updateSnake(now);
}

void Snake1DGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::GameOver) {
    if ((now / 180U) % 2U == 0U) {
      const uint8_t gameOverWidth = Config::GAME_PIXEL_WIDTH * 3U;
      const uint16_t gameOverStart =
          Config::LED_COUNT / 2U - gameOverWidth / 2U;
      for (uint8_t width = 0; width < gameOverWidth; ++width) {
        LedManager::setStripPixel(gameOverStart + width,
                                  Config::SNAKE_ERROR_COLOR);
      }
    }
    return;
  }

  for (uint8_t index = 0; index < lives_; ++index) {
    LedManager::setStripPixel(index, scaleColor(Config::BUTTON_2_COLOR, 96));
  }
  LedManager::setStripPixel(Config::SNAKE_HOME_POSITION, 0x404040UL);

  uint16_t cursor = headPosition_;
  for (uint8_t index = 0; index < segmentCount_; ++index) {
    const Segment& segment = segments_[index];
    const uint8_t cellWidth = segmentWidthCells(segment);
    const uint16_t pixelWidth =
        static_cast<uint16_t>(cellWidth) * Config::GAME_PIXEL_WIDTH;
    for (uint16_t offset = 0; offset < pixelWidth; ++offset) {
      if (cursor + offset >= Config::LED_COUNT) {
        break;
      }

      uint32_t color;
      if (segment.special) {
        const uint8_t rainbowPhase = static_cast<uint8_t>(
            (now / 70U + offset / Config::GAME_PIXEL_WIDTH) % 6U);
        color = rainbowColor(rainbowPhase);
      } else {
        const uint8_t brightness = index == 0 ? 176U : 112U;
        color = scaleColor(colorForIndex(segment.colorIndex), brightness);
      }
      LedManager::setStripPixel(cursor + offset, color);
    }
    cursor += pixelWidth;
    if (cursor >= Config::LED_COUNT) {
      break;
    }
  }

  for (uint8_t index = 0; index < Config::SNAKE_MAX_SHOTS; ++index) {
    const Shot& shot = shots_[index];
    if (!shot.active) {
      continue;
    }
    const uint32_t color = colorForIndex(shot.colorIndex);
    for (uint8_t width = 0; width < Config::GAME_PIXEL_WIDTH; ++width) {
      if (shot.position >= width) {
        LedManager::setStripPixel(shot.position - width, color);
      }
    }
    if (shot.position >= Config::GAME_PIXEL_WIDTH) {
      LedManager::setStripPixel(shot.position - Config::GAME_PIXEL_WIDTH,
                                scaleColor(color, 72));
    }
  }

  for (uint8_t index = 0; index < Config::SNAKE_MAX_BLASTS; ++index) {
    const Blast& blast = blasts_[index];
    if (!blast.active) {
      continue;
    }
    const uint32_t elapsed = now - blast.startedAt;
    const uint8_t pulse = static_cast<uint8_t>(elapsed / 35U);
    const uint8_t radius = static_cast<uint8_t>(
        blast.intensity +
        (elapsed * static_cast<uint16_t>(blast.intensity) * 5U) /
            Config::SNAKE_BLAST_MS);
    const uint8_t brightness = static_cast<uint8_t>(
        255U - (elapsed * 220U) / Config::SNAKE_BLAST_MS);
    const uint32_t color = scaleColor(blast.color, brightness);

    if ((pulse & 0x01U) == 0U && elapsed < Config::SNAKE_BLAST_MS / 2U) {
      const int16_t start = -static_cast<int16_t>(blast.intensity);
      const int16_t end = static_cast<int16_t>(blast.intensity);
      for (int16_t offset = start; offset <= end; ++offset) {
        const int16_t position = static_cast<int16_t>(blast.position) + offset;
        if (position >= 0 &&
            position < static_cast<int16_t>(Config::LED_COUNT)) {
          LedManager::setStripPixel(
              static_cast<uint16_t>(position),
              (offset & 0x01) == 0 ? 0xFFFFFFUL : color);
        }
      }
    }
    if (blast.position >= radius) {
      LedManager::setStripPixel(blast.position - radius, color);
    }
    if (blast.position + radius < Config::LED_COUNT) {
      LedManager::setStripPixel(blast.position + radius, color);
    }
  }
}
