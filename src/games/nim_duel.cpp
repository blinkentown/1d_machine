#include "games/nim_duel.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace {

constexpr uint8_t CELL_COUNT =
    Config::LED_COUNT / Config::GAME_PIXEL_WIDTH;
constexpr uint8_t BOARD_OFFSET =
    (CELL_COUNT - Config::NIM_STARTING_STONES) / 2U;

static_assert(Config::LED_COUNT % Config::GAME_PIXEL_WIDTH == 0,
              "Nim requires complete logical cells");
static_assert(Config::NIM_STARTING_STONES <= CELL_COUNT,
              "Nim stones must fit on the strip");

}  // namespace

void NimDuelGame::start(uint32_t now) {
  scores_[0] = 0;
  scores_[1] = 0;
  roundStarter_ = 0;
  DEBUG_PRINTLN(F("Nim Duel started: encoder selects 1-3, primary takes"));
  startRound(now);
}

void NimDuelGame::startRound(uint32_t now) {
  remaining_ = Config::NIM_STARTING_STONES;
  selectedTake_ = 1;
  activePlayer_ = roundStarter_;
  winner_ = 0;
  phase_ = Phase::Playing;
  phaseChangedAt_ = now;
}

void NimDuelGame::adjustTake(int8_t delta) {
  if (delta == 0) {
    return;
  }
  int16_t next = static_cast<int16_t>(selectedTake_) + delta;
  while (next < 1) {
    next += Config::NIM_MAX_TAKE;
  }
  while (next > Config::NIM_MAX_TAKE) {
    next -= Config::NIM_MAX_TAKE;
  }
  selectedTake_ = static_cast<uint8_t>(next);
  if (selectedTake_ > remaining_) {
    selectedTake_ = remaining_;
  }
}

void NimDuelGame::confirmTake(uint32_t now) {
  const uint8_t take =
      selectedTake_ > remaining_ ? remaining_ : selectedTake_;
  remaining_ -= take;
  if (remaining_ == 0) {
    winner_ = activePlayer_ + 1U;
    if (scores_[activePlayer_] < 999U) {
      ++scores_[activePlayer_];
    }
    phase_ = Phase::RoundWon;
    phaseChangedAt_ = now;
    return;
  }

  activePlayer_ ^= 1U;
  selectedTake_ = 1;
}

void NimDuelGame::update(uint32_t now) {
  if (phase_ == Phase::RoundWon) {
    if (static_cast<uint32_t>(now - phaseChangedAt_) >=
        Config::NIM_ROUND_FEEDBACK_MS) {
      roundStarter_ ^= 1U;
      startRound(now);
    }
    return;
  }

  const InputManager::Encoder activeEncoder =
      activePlayer_ == 0U ? InputManager::Encoder::Player1
                          : InputManager::Encoder::Player2;
  adjustTake(InputManager::encoderDelta(activeEncoder));

  const InputManager::Button confirmButton =
      activePlayer_ == 0U ? InputManager::Button::Red
                          : InputManager::Button::Blue;
  if (InputManager::wasPressed(confirmButton)) {
    confirmTake(now);
  }
}

void NimDuelGame::render(uint32_t now) const {
  LedManager::clearStrip();
  if (phase_ == Phase::RoundWon) {
    const bool bright = ((now - phaseChangedAt_) / 120U) % 2U == 0U;
    if (bright) {
      const uint16_t half = Config::LED_COUNT / 2U;
      LedManager::setStripRange(
          winner_ == 1U ? 0 : half, half,
          winner_ == 1U ? Config::NIM_PLAYER_1_COLOR
                        : Config::NIM_PLAYER_2_COLOR);
    }
    return;
  }

  const uint8_t selectionStart = remaining_ - selectedTake_;
  for (uint8_t stone = 0; stone < remaining_; ++stone) {
    const uint16_t start =
        static_cast<uint16_t>(BOARD_OFFSET + stone) *
        Config::GAME_PIXEL_WIDTH;
    const uint32_t color =
        stone >= selectionStart
            ? activePlayer_ == 0U ? Config::NIM_PLAYER_1_COLOR
                                  : Config::NIM_PLAYER_2_COLOR
            : Config::NIM_STONE_COLOR;
    LedManager::setStripRange(start + 1U, Config::GAME_PIXEL_WIDTH - 2U,
                              color);
  }

  LedManager::setStripPixel(
      0, activePlayer_ == 0U ? Config::NIM_PLAYER_1_COLOR : 0);
  LedManager::setStripPixel(
      Config::LED_COUNT - 1U,
      activePlayer_ == 1U ? Config::NIM_PLAYER_2_COLOR : 0);
}
