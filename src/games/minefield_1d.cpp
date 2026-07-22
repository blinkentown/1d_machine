#include "games/minefield_1d.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t CELL_COUNT =
    Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;
constexpr uint32_t ALL_CELLS = (1UL << CELL_COUNT) - 1UL;

static_assert(Config::LED_COUNT % Config::GAME_PIXEL_WIDTH == 0,
              "Minefield requires complete logical cells");
static_assert(CELL_COUNT < 32, "Minefield uses a 32-bit cell mask");

}  // namespace

void Minefield1DGame::start(uint32_t now) {
  score_ = 0;
  randomState_ = static_cast<uint16_t>(micros()) ^ static_cast<uint16_t>(now);
  if (randomState_ == 0) {
    randomState_ = 1;
  }
  DEBUG_PRINTLN(F("Minefield started: encoder moves, Red reveals, Green flags"));
  generateBoard(now);
}

uint16_t Minefield1DGame::nextRandom() {
  randomState_ ^= randomState_ << 7;
  randomState_ ^= randomState_ >> 9;
  randomState_ ^= randomState_ << 8;
  return randomState_;
}

uint8_t Minefield1DGame::mineCountForScore() const {
  const uint16_t count = Config::MINEFIELD_BASE_MINES +
                         score_ / Config::MINEFIELD_MORE_MINES_EVERY;
  return count > Config::MINEFIELD_MAX_MINES
             ? Config::MINEFIELD_MAX_MINES
             : static_cast<uint8_t>(count);
}

void Minefield1DGame::generateBoard(uint32_t now) {
  mines_ = 0;
  revealed_ = 0;
  flagged_ = 0;
  const uint8_t mineCount = mineCountForScore();
  while (__builtin_popcountl(mines_) < mineCount) {
    mines_ |= 1UL << (nextRandom() % CELL_COUNT);
  }
  cursor_ = CELL_COUNT / 2U;
  firstReveal_ = true;
  phase_ = Phase::Playing;
  phaseChangedAt_ = now;
}

void Minefield1DGame::moveCursor(int8_t delta) {
  int16_t target = static_cast<int16_t>(cursor_) + delta;
  while (target < 0) {
    target += CELL_COUNT;
  }
  while (target >= CELL_COUNT) {
    target -= CELL_COUNT;
  }
  cursor_ = static_cast<uint8_t>(target);
}

uint8_t Minefield1DGame::adjacentMines(uint8_t cell) const {
  uint8_t count = 0;
  if (cell > 0 && (mines_ & (1UL << (cell - 1U))) != 0U) {
    ++count;
  }
  if (cell + 1U < CELL_COUNT &&
      (mines_ & (1UL << (cell + 1U))) != 0U) {
    ++count;
  }
  return count;
}

uint8_t Minefield1DGame::adjacentFlags(uint8_t cell) const {
  uint8_t count = 0;
  if (cell > 0 && (flagged_ & (1UL << (cell - 1U))) != 0U) {
    ++count;
  }
  if (cell + 1U < CELL_COUNT &&
      (flagged_ & (1UL << (cell + 1U))) != 0U) {
    ++count;
  }
  return count;
}

void Minefield1DGame::relocateFirstMine(uint8_t cell) {
  mines_ &= ~(1UL << cell);
  uint8_t replacement = cell;
  while (replacement == cell || (mines_ & (1UL << replacement)) != 0U) {
    replacement = static_cast<uint8_t>(nextRandom() % CELL_COUNT);
  }
  mines_ |= 1UL << replacement;
}

void Minefield1DGame::revealSafeRun(uint8_t cell) {
  revealed_ |= 1UL << cell;
  if (adjacentMines(cell) != 0U) {
    return;
  }

  int8_t scan = static_cast<int8_t>(cell) - 1;
  while (scan >= 0) {
    const uint32_t bit = 1UL << scan;
    if ((flagged_ & bit) != 0U) {
      break;
    }
    revealed_ |= bit;
    if (adjacentMines(static_cast<uint8_t>(scan)) != 0U) {
      break;
    }
    --scan;
  }

  uint8_t right = cell + 1U;
  while (right < CELL_COUNT) {
    const uint32_t bit = 1UL << right;
    if ((flagged_ & bit) != 0U) {
      break;
    }
    revealed_ |= bit;
    if (adjacentMines(right) != 0U) {
      break;
    }
    ++right;
  }
}

void Minefield1DGame::checkSolved(uint32_t now) {
  if (((revealed_ | mines_) & ALL_CELLS) != ALL_CELLS) {
    return;
  }
  if (score_ < 999U) {
    ++score_;
  }
  phase_ = Phase::Solved;
  phaseChangedAt_ = now;
}

void Minefield1DGame::chordReveal(uint8_t cell, uint32_t now) {
  const uint8_t clue = adjacentMines(cell);
  if (clue == 0U || adjacentFlags(cell) != clue) {
    return;
  }

  const uint8_t firstNeighbor = cell == 0 ? 1U : cell - 1U;
  const uint8_t lastNeighbor =
      cell + 1U < CELL_COUNT ? cell + 1U : cell - 1U;
  for (uint8_t neighbor = firstNeighbor; neighbor <= lastNeighbor;
       ++neighbor) {
    if (neighbor == cell) {
      continue;
    }
    const uint32_t bit = 1UL << neighbor;
    if ((flagged_ & bit) != 0U || (revealed_ & bit) != 0U) {
      continue;
    }
    if ((mines_ & bit) != 0U) {
      phase_ = Phase::GameOver;
      phaseChangedAt_ = now;
      return;
    }
    revealSafeRun(neighbor);
  }
  checkSolved(now);
}

void Minefield1DGame::reveal(uint8_t cell, uint32_t now) {
  const uint32_t bit = 1UL << cell;
  if ((flagged_ & bit) != 0U) {
    return;
  }
  if ((revealed_ & bit) != 0U) {
    chordReveal(cell, now);
    return;
  }
  if (firstReveal_) {
    firstReveal_ = false;
    if ((mines_ & bit) != 0U) {
      relocateFirstMine(cell);
    }
  }
  if ((mines_ & bit) != 0U) {
    phase_ = Phase::GameOver;
    phaseChangedAt_ = now;
    return;
  }

  revealSafeRun(cell);
  checkSolved(now);
}

void Minefield1DGame::update(uint32_t now) {
  const int8_t movement =
      InputManager::encoderDelta(InputManager::Encoder::Player1);
  if (phase_ == Phase::GameOver) {
    if (movement != 0 ||
        InputManager::wasPressed(InputManager::Button::Red) ||
        InputManager::wasPressed(InputManager::Button::Green)) {
      start(now);
    }
    return;
  }
  if (phase_ == Phase::Solved) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::MINEFIELD_SUCCESS_MS) {
      generateBoard(now);
    }
    return;
  }

  moveCursor(movement);
  const uint32_t cursorBit = 1UL << cursor_;
  if (InputManager::wasPressed(InputManager::Button::Green) &&
      (revealed_ & cursorBit) == 0U) {
    flagged_ ^= cursorBit;
  }
  if (InputManager::wasPressed(InputManager::Button::Red)) {
    reveal(cursor_, now);
  }
}

void Minefield1DGame::render(uint32_t now) const {
  LedManager::clearStrip();
  if (phase_ == Phase::Solved) {
    uint16_t count = static_cast<uint16_t>(
        (static_cast<uint32_t>(now - phaseChangedAt_) * Config::LED_COUNT) /
        Config::MINEFIELD_SUCCESS_MS);
    if (count > Config::LED_COUNT) {
      count = Config::LED_COUNT;
    }
    LedManager::setStripRange(0, count, Config::MINEFIELD_SUCCESS_COLOR);
    return;
  }

  for (uint8_t cell = 0; cell < CELL_COUNT; ++cell) {
    const uint32_t bit = 1UL << cell;
    uint32_t color = Config::MINEFIELD_HIDDEN_COLOR;
    if (phase_ == Phase::GameOver && (mines_ & bit) != 0U) {
      color = Config::MINEFIELD_MINE_COLOR;
    } else if ((flagged_ & bit) != 0U) {
      color = Config::MINEFIELD_FLAG_COLOR;
    } else if ((revealed_ & bit) != 0U) {
      const uint8_t adjacent = adjacentMines(cell);
      color = adjacent == 0U
                  ? Config::MINEFIELD_CLEAR_COLOR
                  : adjacent == 1U ? Config::MINEFIELD_ONE_COLOR
                                   : Config::MINEFIELD_TWO_COLOR;
    }

    const uint16_t start =
        static_cast<uint16_t>(cell) * Config::GAME_PIXEL_WIDTH;
    LedManager::setStripRange(start + 1U, Config::GAME_PIXEL_WIDTH - 2U,
                              color);
    if (cell == cursor_) {
      LedManager::setStripPixel(start, Config::MINEFIELD_CURSOR_COLOR);
      LedManager::setStripPixel(start + Config::GAME_PIXEL_WIDTH - 1U,
                                Config::MINEFIELD_CURSOR_COLOR);
    }
  }
}
