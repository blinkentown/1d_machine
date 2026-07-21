#include "games/memory_sequence.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

void MemorySequenceGame::start(uint32_t now) {
  sequenceSeed_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (sequenceSeed_ == 0) {
    sequenceSeed_ = 1;
  }
  length_ = Config::MEMORY_START_LENGTH;
  won_ = false;
  DEBUG_PRINTLN(F("Memory Sequence started: repeat R/G/B/Y"));
  beginShowing(now);
}

uint16_t MemorySequenceGame::advanceRandom(uint16_t state) {
  state ^= state << 7;
  state ^= state >> 9;
  state ^= state << 8;
  return state;
}

uint8_t MemorySequenceGame::sequenceColor(uint8_t index) const {
  uint16_t state = sequenceSeed_;
  do {
    state = advanceRandom(state);
  } while (index-- > 0);
  return static_cast<uint8_t>(state & 0x03U);
}

int8_t MemorySequenceGame::pressedColor() {
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

uint32_t MemorySequenceGame::colorFor(uint8_t color) {
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

uint8_t MemorySequenceGame::stationCell(uint8_t color) {
  return 3U + color * 6U;
}

void MemorySequenceGame::beginShowing(uint32_t now) {
  inputIndex_ = 0;
  phaseChangedAt_ = now;
  phase_ = Phase::Showing;
}

void MemorySequenceGame::fail(uint32_t now) {
  phase_ = Phase::GameOver;
  phaseChangedAt_ = now;
}

void MemorySequenceGame::update(uint32_t now) {
  if (phase_ == Phase::GameOver) {
    if (pressedColor() >= 0) {
      start(now);
    }
    return;
  }

  if (phase_ == Phase::Showing) {
    const uint16_t slotMs =
        Config::MEMORY_SHOW_ON_MS + Config::MEMORY_SHOW_GAP_MS;
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        static_cast<uint32_t>(length_) * slotMs) {
      phase_ = Phase::PlayerInput;
      phaseChangedAt_ = now;
      lastInputAt_ = now;
    }
    return;
  }

  if (phase_ == Phase::RoundSuccess) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) <
        Config::MEMORY_SUCCESS_MS) {
      return;
    }
    if (length_ >= Config::MEMORY_MAX_LENGTH) {
      won_ = true;
      phase_ = Phase::GameOver;
      phaseChangedAt_ = now;
    } else {
      ++length_;
      beginShowing(now);
    }
    return;
  }

  if (static_cast<uint32_t>(now - lastInputAt_) >=
      Config::MEMORY_INPUT_TIMEOUT_MS) {
    fail(now);
    return;
  }

  const int8_t color = pressedColor();
  if (color < 0) {
    return;
  }
  lastColor_ = static_cast<uint8_t>(color);
  lastInputAt_ = now;
  if (lastColor_ != sequenceColor(inputIndex_)) {
    fail(now);
    return;
  }

  ++inputIndex_;
  if (inputIndex_ >= length_) {
    phase_ = Phase::RoundSuccess;
    phaseChangedAt_ = now;
  }
}

void MemorySequenceGame::render(uint32_t now) const {
  LedManager::clearStrip();

  if (phase_ == Phase::GameOver) {
    if ((now / 170U) % 2U == 0U) {
      if (won_) {
        for (uint8_t color = 0; color < 4; ++color) {
          LedManager::setStripRange(color * (Config::LED_COUNT / 4U),
                                    Config::LED_COUNT / 4U, colorFor(color));
        }
      } else {
        LedManager::setStripRange(Config::LED_COUNT / 2U -
                                      Config::EXPLOSION_INTENSITY,
                                  Config::EXPLOSION_INTENSITY * 2U,
                                  Config::MEMORY_ERROR_COLOR);
      }
    }
    return;
  }

  for (uint8_t color = 0; color < 4; ++color) {
    LedManager::setGameCell(stationCell(color),
                            colorFor(color) & 0x202020UL);
  }

  if (phase_ == Phase::Showing) {
    const uint16_t slotMs =
        Config::MEMORY_SHOW_ON_MS + Config::MEMORY_SHOW_GAP_MS;
    const uint32_t elapsed = now - phaseChangedAt_;
    const uint8_t showIndex = elapsed / slotMs;
    if (showIndex < length_ && elapsed % slotMs < Config::MEMORY_SHOW_ON_MS) {
      const uint8_t color = sequenceColor(showIndex);
      LedManager::setGameCell(stationCell(color), colorFor(color));
    }
  } else if (phase_ == Phase::PlayerInput && inputIndex_ > 0 &&
             static_cast<uint32_t>(now - lastInputAt_) <
                 Config::MEMORY_INPUT_FEEDBACK_MS) {
    LedManager::setGameCell(stationCell(lastColor_), colorFor(lastColor_));
  } else if (phase_ == Phase::RoundSuccess) {
    const uint16_t count = static_cast<uint16_t>(
        (static_cast<uint32_t>(now - phaseChangedAt_) * Config::LED_COUNT) /
        Config::MEMORY_SUCCESS_MS);
    LedManager::setStripRange(0, count, Config::MEMORY_SUCCESS_COLOR);
  }
}
