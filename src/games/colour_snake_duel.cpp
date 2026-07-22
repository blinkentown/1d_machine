#include "games/colour_snake_duel.h"

#include "controls.h"
#include "led_manager.h"

static_assert(Config::LED_COUNT % Config::GAME_PIXEL_WIDTH == 0,
              "Colour Snake requires complete logical cells");
static_assert((Config::LED_COUNT / Config::GAME_PIXEL_WIDTH) % 2U == 0U,
              "Colour Snake requires equal player halves");

void ColourSnakeDuelGame::start(uint32_t now) {
  player1Score_ = 0;
  player2Score_ = 0;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(F("Colour Snake Duel started"));
  DEBUG_PRINTLN(F("P1 Red/Green, P2 Blue/Yellow"));
  startRound(now);
}

void ColourSnakeDuelGame::startRound(uint32_t now) {
  player1LengthPixels_ = 0;
  player2LengthPixels_ = 0;
  player1PenaltyPixels_ = 0;
  player2PenaltyPixels_ = 0;
  player1Shot_.active = false;
  player2Shot_.active = false;
  player1Effect_.active = false;
  player2Effect_.active = false;
  const uint8_t initialPixels =
      Config::COLOUR_SNAKE_INITIAL_LENGTH * Config::GAME_PIXEL_WIDTH;
  for (uint8_t index = 0; index < initialPixels;
       ++index) {
    growSidePixel(true);
    growSidePixel(false);
  }
  roundStartedAt_ = now;
  lastGrowthAt_ = now;
  lastPenaltyStepAt_ = now;
  phaseChangedAt_ = now;
  roundWinner_ = 0;
  phase_ = Phase::Playing;
}

uint16_t ColourSnakeDuelGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

uint32_t ColourSnakeDuelGame::colorFor(bool player1, uint8_t colorIndex) {
  if (player1) {
    return colorIndex == 0U ? Config::BUTTON_1_COLOR
                            : Config::BUTTON_2_COLOR;
  }
  return colorIndex == 0U ? Config::BUTTON_3_COLOR
                          : Config::BUTTON_4_COLOR;
}

uint32_t ColourSnakeDuelGame::scaleColor(uint32_t color, uint8_t scale) {
  const uint8_t red = static_cast<uint8_t>((color >> 16U) & 0xFFU);
  const uint8_t green = static_cast<uint8_t>((color >> 8U) & 0xFFU);
  const uint8_t blue = static_cast<uint8_t>(color & 0xFFU);
  return (static_cast<uint32_t>(
              (static_cast<uint16_t>(red) * scale) / 255U)
          << 16U) |
         (static_cast<uint32_t>(
              (static_cast<uint16_t>(green) * scale) / 255U)
          << 8U) |
         (static_cast<uint16_t>(blue) * scale) / 255U;
}

bool ColourSnakeDuelGame::growSidePixel(bool player1) {
  constexpr uint8_t HALF_PIXELS = Config::LED_COUNT / 2U;
  uint8_t& length =
      player1 ? player1LengthPixels_ : player2LengthPixels_;
  uint8_t* colors = player1 ? player1Colors_ : player2Colors_;
  if (length >= HALF_PIXELS) {
    return true;
  }

  const uint8_t oldSegmentCount = static_cast<uint8_t>(
      (length + Config::GAME_PIXEL_WIDTH - 1U) / Config::GAME_PIXEL_WIDTH);
  ++length;
  const uint8_t newSegmentCount = static_cast<uint8_t>(
      (length + Config::GAME_PIXEL_WIDTH - 1U) / Config::GAME_PIXEL_WIDTH);
  if (newSegmentCount > oldSegmentCount) {
    colors[oldSegmentCount] = static_cast<uint8_t>(nextRandom() & 0x01U);
  }
  return length >= HALF_PIXELS;
}

void ColourSnakeDuelGame::removeHead(bool player1) {
  uint8_t& length =
      player1 ? player1LengthPixels_ : player2LengthPixels_;
  uint8_t* colors = player1 ? player1Colors_ : player2Colors_;
  if (length == 0U) {
    return;
  }
  const uint8_t segmentCount = static_cast<uint8_t>(
      (length + Config::GAME_PIXEL_WIDTH - 1U) / Config::GAME_PIXEL_WIDTH);
  for (uint8_t index = 1; index < segmentCount; ++index) {
    colors[index - 1U] = colors[index];
  }
  length = length > Config::GAME_PIXEL_WIDTH
               ? length - Config::GAME_PIXEL_WIDTH
               : 0U;
}

void ColourSnakeDuelGame::launchShot(bool player1, uint8_t colorIndex,
                                     uint32_t now) {
  Shot& shot = player1 ? player1Shot_ : player2Shot_;
  const uint8_t length =
      player1 ? player1LengthPixels_ : player2LengthPixels_;
  if (shot.active || length == 0U) {
    return;
  }
  shot.position = player1 ? 0U : Config::LED_COUNT - 1U;
  shot.colorIndex = colorIndex;
  shot.lastStepAt = now;
  shot.active = true;
}

void ColourSnakeDuelGame::startHitEffect(bool player1, uint16_t position,
                                         uint8_t colorIndex, uint32_t now) {
  HitEffect& effect = player1 ? player1Effect_ : player2Effect_;
  effect.startedAt = now;
  effect.position = position;
  effect.colorIndex = colorIndex;
  effect.active = true;
}

void ColourSnakeDuelGame::updateShot(Shot& shot, bool player1, uint32_t now) {
  if (!shot.active) {
    return;
  }

  const uint16_t interval =
      Config::gameplayInterval(Config::COLOUR_SNAKE_SHOT_STEP_MS);
  uint32_t dueSteps = (now - shot.lastStepAt) / interval;
  if (dueSteps > 32U) {
    dueSteps = 32U;
  }
  uint8_t steps = static_cast<uint8_t>(dueSteps);

  while (steps-- > 0U && shot.active) {
    shot.lastStepAt += interval;
    const uint8_t length =
        player1 ? player1LengthPixels_ : player2LengthPixels_;
    if (length == 0U) {
      shot.active = false;
      break;
    }
    const uint16_t headPosition =
        player1 ? Config::LED_COUNT / 2U - length
                : Config::LED_COUNT / 2U + length - 1U;
    const bool reachedHead =
        player1 ? shot.position >= headPosition
                : shot.position <= headPosition;
    if (!reachedHead) {
      shot.position += player1 ? 1 : -1;
      continue;
    }

    const uint8_t* colors = player1 ? player1Colors_ : player2Colors_;
    shot.active = false;
    if (shot.colorIndex == colors[0]) {
      startHitEffect(player1, headPosition, shot.colorIndex, now);
      removeHead(player1);
      DEBUG_PRINTLN(player1 ? F("P1 match") : F("P2 match"));
      return;
    }

    startHitEffect(player1, headPosition, 2U, now);
    uint8_t& penalty =
        player1 ? player1PenaltyPixels_ : player2PenaltyPixels_;
    if (player1PenaltyPixels_ == 0U && player2PenaltyPixels_ == 0U) {
      lastPenaltyStepAt_ = now;
    }
    const uint16_t increasedPenalty =
        static_cast<uint16_t>(penalty) + Config::GAME_PIXEL_WIDTH;
    penalty = increasedPenalty > Config::LED_COUNT / 2U
                  ? Config::LED_COUNT / 2U
                  : static_cast<uint8_t>(increasedPenalty);
    DEBUG_PRINTLN(player1 ? F("P1 wrong color") : F("P2 wrong color"));
    return;
  }
}

uint16_t ColourSnakeDuelGame::growthStepInterval(uint32_t now) const {
  uint8_t speedSteps = static_cast<uint8_t>(
      (now - roundStartedAt_) / Config::COLOUR_SNAKE_SPEEDUP_EVERY_MS);
  if (speedSteps > Config::COLOUR_SNAKE_MAX_SPEED_STEPS) {
    speedSteps = Config::COLOUR_SNAKE_MAX_SPEED_STEPS;
  }
  const uint16_t interval =
      Config::COLOUR_SNAKE_INITIAL_PIXEL_STEP_MS -
      static_cast<uint16_t>(speedSteps) * Config::COLOUR_SNAKE_SPEEDUP_MS;
  return Config::gameplayInterval(interval);
}

void ColourSnakeDuelGame::finishRound(bool player1Breached,
                                      bool player2Breached, uint32_t now) {
  if (player1Breached && player2Breached) {
    roundWinner_ = 0;
  } else if (player1Breached) {
    roundWinner_ = 1;
    if (player2Score_ < 255U) {
      ++player2Score_;
    }
  } else {
    roundWinner_ = -1;
    if (player1Score_ < 255U) {
      ++player1Score_;
    }
  }

  player1Shot_.active = false;
  player2Shot_.active = false;
  phaseChangedAt_ = now;
  phase_ = player1Score_ >= Config::COLOUR_SNAKE_ROUNDS_TO_WIN ||
                   player2Score_ >= Config::COLOUR_SNAKE_ROUNDS_TO_WIN
               ? Phase::GameOver
               : Phase::RoundResult;
}

void ColourSnakeDuelGame::updateEffects(uint32_t now) {
  if (player1Effect_.active &&
      static_cast<uint32_t>(now - player1Effect_.startedAt) >=
          Config::COLOUR_SNAKE_HIT_EFFECT_MS) {
    player1Effect_.active = false;
  }
  if (player2Effect_.active &&
      static_cast<uint32_t>(now - player2Effect_.startedAt) >=
          Config::COLOUR_SNAKE_HIT_EFFECT_MS) {
    player2Effect_.active = false;
  }
}

bool ColourSnakeDuelGame::anyPlayerButtonPressed() {
  return Controls::primaryPressed(Controls::Player::One) ||
         Controls::secondaryPressed(Controls::Player::One) ||
         Controls::primaryPressed(Controls::Player::Two) ||
         Controls::secondaryPressed(Controls::Player::Two);
}

void ColourSnakeDuelGame::update(uint32_t now) {
  updateEffects(now);

  if (phase_ == Phase::GameOver) {
    if (anyPlayerButtonPressed()) {
      start(now);
    }
    return;
  }
  if (phase_ == Phase::RoundResult) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::COLOUR_SNAKE_ROUND_RESULT_MS) {
      startRound(now);
    }
    return;
  }

  if (Controls::primaryPressed(Controls::Player::One)) {
    launchShot(true, 0U, now);
  } else if (Controls::secondaryPressed(Controls::Player::One)) {
    launchShot(true, 1U, now);
  }
  if (Controls::primaryPressed(Controls::Player::Two)) {
    launchShot(false, 0U, now);
  } else if (Controls::secondaryPressed(Controls::Player::Two)) {
    launchShot(false, 1U, now);
  }

  updateShot(player1Shot_, true, now);
  updateShot(player2Shot_, false, now);

  bool player1Breached = false;
  bool player2Breached = false;
  bool penaltyStepped = false;
  if ((player1PenaltyPixels_ > 0U || player2PenaltyPixels_ > 0U) &&
      static_cast<uint32_t>(now - lastPenaltyStepAt_) >=
          Config::COLOUR_SNAKE_PENALTY_STEP_MS) {
    lastPenaltyStepAt_ = now;
    if (player1PenaltyPixels_ > 0U) {
      --player1PenaltyPixels_;
      player1Breached = growSidePixel(true);
      penaltyStepped = true;
    }
    if (player2PenaltyPixels_ > 0U) {
      --player2PenaltyPixels_;
      player2Breached = growSidePixel(false);
      penaltyStepped = true;
    }
  }

  if (!player1Breached && !player2Breached && !penaltyStepped &&
      static_cast<uint32_t>(now - lastGrowthAt_) >=
          growthStepInterval(now)) {
    lastGrowthAt_ = now;
    player1Breached = growSidePixel(true);
    player2Breached = growSidePixel(false);
  }

  if (player1Breached || player2Breached) {
    finishRound(player1Breached, player2Breached, now);
  }
}

void ColourSnakeDuelGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::GameOver) {
    const uint32_t winnerColor =
        player1Score_ > player2Score_ ? Config::BUTTON_1_COLOR
                                      : Config::BUTTON_3_COLOR;
    if (((now / 120U) & 1U) == 0U) {
      for (uint8_t cell = 0; cell < CELL_COUNT; ++cell) {
        LedManager::setGameCell(cell,
                                (cell & 1U) == 0U ? winnerColor : 0xFFFFFFUL);
      }
    }
    return;
  }

  if (phase_ == Phase::RoundResult) {
    if (((now / 120U) & 1U) == 0U) {
      if (roundWinner_ <= 0) {
        LedManager::setStripRange(
            0U, Config::LED_COUNT / 2U,
            roundWinner_ < 0 ? Config::BUTTON_1_COLOR : 0xFFFFFFUL);
      }
      if (roundWinner_ >= 0) {
        LedManager::setStripRange(
            Config::LED_COUNT / 2U, Config::LED_COUNT / 2U,
            roundWinner_ > 0 ? Config::BUTTON_3_COLOR : 0xFFFFFFUL);
      }
    }
    return;
  }

  LedManager::setStripPixel(0U, 0x300000UL);
  LedManager::setStripPixel(Config::LED_COUNT - 1U, 0x000030UL);

  const uint8_t player1SegmentCount = static_cast<uint8_t>(
      (player1LengthPixels_ + Config::GAME_PIXEL_WIDTH - 1U) /
      Config::GAME_PIXEL_WIDTH);
  for (uint8_t index = 0; index < player1SegmentCount; ++index) {
    const uint8_t consumed = index * Config::GAME_PIXEL_WIDTH;
    const uint8_t remaining = player1LengthPixels_ - consumed;
    const uint8_t width = remaining < Config::GAME_PIXEL_WIDTH
                              ? remaining
                              : Config::GAME_PIXEL_WIDTH;
    const uint16_t start = Config::LED_COUNT / 2U - player1LengthPixels_ +
                           consumed;
    const uint8_t scale = index == 0U
                              ? static_cast<uint8_t>(160U + (now / 8U) % 96U)
                              : 112U;
    LedManager::setStripRange(
        start, width,
        scaleColor(colorFor(true, player1Colors_[index]), scale));
  }
  const uint8_t player2SegmentCount = static_cast<uint8_t>(
      (player2LengthPixels_ + Config::GAME_PIXEL_WIDTH - 1U) /
      Config::GAME_PIXEL_WIDTH);
  for (uint8_t index = 0; index < player2SegmentCount; ++index) {
    const uint8_t consumed = index * Config::GAME_PIXEL_WIDTH;
    const uint8_t remaining = player2LengthPixels_ - consumed;
    const uint8_t width = remaining < Config::GAME_PIXEL_WIDTH
                              ? remaining
                              : Config::GAME_PIXEL_WIDTH;
    const uint16_t outerPosition = Config::LED_COUNT / 2U +
                                   player2LengthPixels_ - 1U - consumed;
    const uint16_t start = outerPosition + 1U - width;
    const uint8_t scale = index == 0U
                              ? static_cast<uint8_t>(160U + (now / 8U) % 96U)
                              : 112U;
    LedManager::setStripRange(
        start, width,
        scaleColor(colorFor(false, player2Colors_[index]), scale));
  }

  if (player1Shot_.active) {
    for (uint8_t offset = 0; offset < Config::TAPE_PIXEL_WIDTH; ++offset) {
      if (player1Shot_.position >= offset) {
        LedManager::setStripPixel(
            player1Shot_.position - offset,
            colorFor(true, player1Shot_.colorIndex));
      }
    }
  }
  if (player2Shot_.active) {
    for (uint8_t offset = 0; offset < Config::TAPE_PIXEL_WIDTH; ++offset) {
      if (player2Shot_.position + offset < Config::LED_COUNT) {
        LedManager::setStripPixel(
            player2Shot_.position + offset,
            colorFor(false, player2Shot_.colorIndex));
      }
    }
  }

  const HitEffect* effects[2] = {&player1Effect_, &player2Effect_};
  for (uint8_t index = 0; index < 2U; ++index) {
    const HitEffect& effect = *effects[index];
    if (!effect.active) {
      continue;
    }
    const uint32_t elapsed = now - effect.startedAt;
    const uint8_t radius = 1U + static_cast<uint8_t>(
                                   elapsed * Config::EXPLOSION_INTENSITY /
                                   Config::COLOUR_SNAKE_HIT_EFFECT_MS);
    const uint16_t center = effect.position;
    const uint32_t color =
        effect.colorIndex < 2U
            ? colorFor(index == 0U, effect.colorIndex)
            : Config::COLOUR_SNAKE_ERROR_COLOR;
    for (uint8_t offset = 0; offset < radius; ++offset) {
      const uint32_t flashColor =
          ((elapsed / 30U + offset) & 1U) == 0U ? 0xFFFFFFUL : color;
      if (center >= offset) {
        LedManager::setStripPixel(center - offset, flashColor);
      }
      if (center + offset < Config::LED_COUNT) {
        LedManager::setStripPixel(center + offset, flashColor);
      }
    }
  }
}
