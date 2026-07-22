#include "games/hanoi_1d.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t PEG_COUNT = 3;
constexpr uint16_t PEG_WIDTH = Config::LED_COUNT / PEG_COUNT;
constexpr uint8_t PEG_CELLS = PEG_WIDTH / Config::GAME_PIXEL_WIDTH;

static_assert(Config::LED_COUNT % PEG_COUNT == 0,
              "Hanoi requires three equal peg zones");
static_assert(PEG_WIDTH % Config::GAME_PIXEL_WIDTH == 0,
              "Hanoi peg zones require complete logical cells");
static_assert(Config::HANOI_MAX_DISKS <= PEG_CELLS,
              "Hanoi disks must fit inside each peg zone");

}  // namespace

void Hanoi1DGame::start(uint32_t now) {
  score_ = 0;
  diskCount_ = Config::HANOI_STARTING_DISKS;
  DEBUG_PRINTLN(F("Hanoi started: R/G/B select pegs, Yellow resets"));
  resetLevel(now);
}

uint8_t Hanoi1DGame::diskPeg(uint8_t disk) const {
  return static_cast<uint8_t>((positions_ >> (disk * 2U)) & 0x03U);
}

void Hanoi1DGame::setDiskPeg(uint8_t disk, uint8_t peg) {
  const uint16_t shift = disk * 2U;
  positions_ = static_cast<uint16_t>(
      (positions_ & ~(0x03U << shift)) |
      (static_cast<uint16_t>(peg) << shift));
}

int8_t Hanoi1DGame::topDisk(uint8_t peg) const {
  for (uint8_t disk = 0; disk < diskCount_; ++disk) {
    if (diskPeg(disk) == peg) {
      return static_cast<int8_t>(disk);
    }
  }
  return -1;
}

void Hanoi1DGame::resetLevel(uint32_t now) {
  positions_ = 0;
  selectedPeg_ = -1;
  phase_ = Phase::Playing;
  phaseChangedAt_ = now;
}

bool Hanoi1DGame::levelComplete() const {
  for (uint8_t disk = 0; disk < diskCount_; ++disk) {
    if (diskPeg(disk) != 2U) {
      return false;
    }
  }
  return true;
}

void Hanoi1DGame::choosePeg(uint8_t peg, uint32_t now) {
  if (selectedPeg_ < 0) {
    if (topDisk(peg) >= 0) {
      selectedPeg_ = static_cast<int8_t>(peg);
    }
    return;
  }

  if (peg == static_cast<uint8_t>(selectedPeg_)) {
    selectedPeg_ = -1;
    return;
  }

  const int8_t movingDisk = topDisk(static_cast<uint8_t>(selectedPeg_));
  const int8_t destinationDisk = topDisk(peg);
  if (movingDisk < 0 ||
      (destinationDisk >= 0 && movingDisk > destinationDisk)) {
    feedbackPeg_ = peg;
    selectedPeg_ = -1;
    phase_ = Phase::Feedback;
    phaseChangedAt_ = now;
    return;
  }

  setDiskPeg(static_cast<uint8_t>(movingDisk), peg);
  selectedPeg_ = -1;
  if (levelComplete()) {
    if (score_ < 999U) {
      ++score_;
    }
    phase_ = Phase::LevelClear;
    phaseChangedAt_ = now;
  }
}

void Hanoi1DGame::update(uint32_t now) {
  if (phase_ == Phase::Feedback) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::HANOI_ERROR_MS) {
      phase_ = Phase::Playing;
    }
    return;
  }
  if (phase_ == Phase::LevelClear) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::HANOI_LEVEL_CLEAR_MS) {
      if (diskCount_ < Config::HANOI_MAX_DISKS) {
        ++diskCount_;
      }
      resetLevel(now);
    }
    return;
  }

  if (InputManager::wasPressed(InputManager::Button::Yellow)) {
    resetLevel(now);
    return;
  }
  if (InputManager::wasPressed(InputManager::Button::Red)) {
    choosePeg(0, now);
  } else if (InputManager::wasPressed(InputManager::Button::Green)) {
    choosePeg(1, now);
  } else if (InputManager::wasPressed(InputManager::Button::Blue)) {
    choosePeg(2, now);
  }
}

uint32_t Hanoi1DGame::diskColor(uint8_t disk) {
  switch (disk) {
    case 0:
      return 0xFFFFFFUL;
    case 1:
      return 0xFF0000UL;
    case 2:
      return 0xFFFF00UL;
    case 3:
      return 0x00FF00UL;
    default:
      return 0x0080FFUL;
  }
}

void Hanoi1DGame::render(uint32_t now) const {
  LedManager::clearStrip();
  if (phase_ == Phase::LevelClear) {
    uint16_t count = static_cast<uint16_t>(
        (static_cast<uint32_t>(now - phaseChangedAt_) * Config::LED_COUNT) /
        Config::HANOI_LEVEL_CLEAR_MS);
    if (count > Config::LED_COUNT) {
      count = Config::LED_COUNT;
    }
    LedManager::setStripRange(0, count, Config::HANOI_SUCCESS_COLOR);
    return;
  }

  for (uint8_t peg = 0; peg < PEG_COUNT; ++peg) {
    const uint16_t pegStart = static_cast<uint16_t>(peg) * PEG_WIDTH;
    LedManager::setStripPixel(pegStart,
                              peg == 2U ? Config::HANOI_GOAL_COLOR
                                        : Config::HANOI_PEG_COLOR);
    LedManager::setStripPixel(pegStart + PEG_WIDTH - 1U,
                              peg == 2U ? Config::HANOI_GOAL_COLOR
                                        : Config::HANOI_PEG_COLOR);

    uint8_t stackHeight = 0;
    for (int8_t disk = static_cast<int8_t>(diskCount_) - 1; disk >= 0;
         --disk) {
      if (diskPeg(static_cast<uint8_t>(disk)) != peg) {
        continue;
      }
      const uint16_t cell = PEG_CELLS - 1U - stackHeight;
      const uint16_t start = pegStart + cell * Config::GAME_PIXEL_WIDTH;
      LedManager::setStripRange(start + 1U, Config::GAME_PIXEL_WIDTH - 2U,
                                diskColor(static_cast<uint8_t>(disk)));
      ++stackHeight;
    }

    if (selectedPeg_ == static_cast<int8_t>(peg)) {
      LedManager::setStripRange(pegStart, Config::GAME_PIXEL_WIDTH / 2U,
                                Config::HANOI_SELECTION_COLOR);
    }
  }

  if (phase_ == Phase::Feedback &&
      ((now - phaseChangedAt_) / 80U) % 2U == 0U) {
    LedManager::setStripRange(static_cast<uint16_t>(feedbackPeg_) * PEG_WIDTH,
                              PEG_WIDTH, Config::HANOI_ERROR_COLOR);
  }
}
