#include "games/codebreaker.h"

#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint16_t GUESS_WIDTH =
    Config::CODEBREAKER_CODE_LENGTH * Config::GAME_PIXEL_WIDTH;
constexpr uint16_t GUESS_START = Config::LED_COUNT / 2U - GUESS_WIDTH / 2U;
constexpr uint16_t FEEDBACK_START = GUESS_START - GUESS_WIDTH -
                                    Config::GAME_PIXEL_WIDTH;

}  // namespace

void CodebreakerGame::start(uint32_t now) {
  score_ = 0;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(F("Codebreaker started: enter four R/G/B/Y colors"));
  startRound(now);
}

uint16_t CodebreakerGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

uint32_t CodebreakerGame::colorFor(uint8_t color) {
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

int8_t CodebreakerGame::pressedColor() {
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

uint8_t CodebreakerGame::secretColor(uint8_t index) const {
  return static_cast<uint8_t>((secretBits_ >> (index * 2U)) & 0x03U);
}

void CodebreakerGame::startRound(uint32_t now) {
  secretBits_ = 0;
  for (uint8_t index = 0; index < Config::CODEBREAKER_CODE_LENGTH; ++index) {
    secretBits_ |= static_cast<uint16_t>(nextRandom() & 0x03U) << (index * 2U);
  }
  inputCount_ = 0;
  attemptsLeft_ = Config::CODEBREAKER_ATTEMPTS;
  exactMatches_ = 0;
  misplacedMatches_ = 0;
  phaseChangedAt_ = now;
  phase_ = Phase::Input;
}

void CodebreakerGame::evaluateGuess(uint32_t now) {
  exactMatches_ = 0;
  misplacedMatches_ = 0;
  for (uint8_t index = 0; index < Config::CODEBREAKER_CODE_LENGTH; ++index) {
    if (guess_[index] == secretColor(index)) {
      ++exactMatches_;
    }
  }

  for (uint8_t color = 0; color < 4U; ++color) {
    uint8_t secretCount = 0;
    uint8_t guessCount = 0;
    for (uint8_t index = 0; index < Config::CODEBREAKER_CODE_LENGTH; ++index) {
      if (guess_[index] == secretColor(index)) {
        continue;
      }
      if (secretColor(index) == color) {
        ++secretCount;
      }
      if (guess_[index] == color) {
        ++guessCount;
      }
    }
    misplacedMatches_ += secretCount < guessCount ? secretCount : guessCount;
  }

  phaseChangedAt_ = now;
  if (exactMatches_ == Config::CODEBREAKER_CODE_LENGTH) {
    if (score_ < 999U) {
      ++score_;
    }
    phase_ = Phase::RoundSuccess;
    return;
  }
  if (attemptsLeft_ > 0) {
    --attemptsLeft_;
  }
  phase_ = Phase::Feedback;
}

void CodebreakerGame::update(uint32_t now) {
  if (phase_ == Phase::GameOver) {
    if (pressedColor() >= 0) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::RoundSuccess) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::CODEBREAKER_SUCCESS_MS) {
      startRound(now);
    }
    return;
  }

  if (phase_ == Phase::Feedback) {
    const int8_t color = pressedColor();
    if (color < 0) {
      return;
    }
    if (attemptsLeft_ == 0) {
      phase_ = Phase::GameOver;
      phaseChangedAt_ = now;
    } else {
      inputCount_ = 1;
      guess_[0] = static_cast<uint8_t>(color);
      phase_ = Phase::Input;
      phaseChangedAt_ = now;
    }
    return;
  }

  const int8_t color = pressedColor();
  if (color < 0) {
    return;
  }
  guess_[inputCount_++] = static_cast<uint8_t>(color);
  if (inputCount_ >= Config::CODEBREAKER_CODE_LENGTH) {
    evaluateGuess(now);
  }
}

void CodebreakerGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::RoundSuccess) {
    uint16_t count = static_cast<uint16_t>(
        (static_cast<uint32_t>(now - phaseChangedAt_) * Config::LED_COUNT) /
        Config::CODEBREAKER_SUCCESS_MS);
    if (count > Config::LED_COUNT) {
      count = Config::LED_COUNT;
    }
    LedManager::setStripRange(0, count, Config::CODEBREAKER_EXACT_COLOR);
    return;
  }

  const bool revealSecret = phase_ == Phase::GameOver;
  for (uint8_t index = 0; index < Config::CODEBREAKER_CODE_LENGTH; ++index) {
    const uint32_t color = revealSecret
                               ? colorFor(secretColor(index))
                               : index < inputCount_ ? colorFor(guess_[index])
                                                     : Config::CODEBREAKER_SLOT_COLOR;
    LedManager::setStripRange(
        GUESS_START + static_cast<uint16_t>(index) * Config::GAME_PIXEL_WIDTH,
        Config::GAME_PIXEL_WIDTH, color);
  }

  if (phase_ == Phase::Feedback || phase_ == Phase::GameOver) {
    for (uint8_t index = 0; index < Config::CODEBREAKER_CODE_LENGTH; ++index) {
      uint32_t color = Config::CODEBREAKER_MISS_COLOR;
      if (index < exactMatches_) {
        color = Config::CODEBREAKER_EXACT_COLOR;
      } else if (index < exactMatches_ + misplacedMatches_) {
        color = Config::CODEBREAKER_MISPLACED_COLOR;
      }
      LedManager::setStripRange(
          FEEDBACK_START +
              static_cast<uint16_t>(index) * Config::GAME_PIXEL_WIDTH,
          Config::GAME_PIXEL_WIDTH, color);
    }
  }

  for (uint8_t attempt = 0; attempt < attemptsLeft_; ++attempt) {
    LedManager::setStripPixel(attempt, Config::CODEBREAKER_ATTEMPT_COLOR);
  }
}
