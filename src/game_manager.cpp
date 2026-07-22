#include "game_manager.h"

#include <avr/pgmspace.h>

#include "config.h"
#include "display_manager.h"
#if GAME_SET_SOURCE_GAMES
#include "games/catch_1d.h"
#include "games/colour_gate.h"
#else
#include "games/colour_snake_duel.h"
#include "games/colour_shooter.h"
#include "games/reaction_race.h"
#include "games/tennis_1d.h"
#endif
#include "games/pong_1d.h"
#include "games/twang.h"
#include "input_manager.h"
#include "led_manager.h"
#include "power_mode_manager.h"
#include "power_stress_test.h"

namespace GameManager {
namespace {

using GameId = DisplayManager::Mode;

enum class State : uint8_t {
  Selecting,
  PowerStress,
  Running,
};

#if GAME_SET_SOURCE_GAMES
const GameId GAME_CATALOG[] PROGMEM = {
    GameId::Catch1D,
    GameId::ColourGate,
};
#else
const GameId GAME_CATALOG[] PROGMEM = {
    GameId::Twang,
    GameId::ColourShooter,
    GameId::Pong1D,
    GameId::Tennis1D,
    GameId::ReactionRace,
    GameId::ColourSnakeDuel,
};
#endif
constexpr uint8_t GAME_COUNT = sizeof(GAME_CATALOG) / sizeof(GAME_CATALOG[0]);

#if GAME_SET_SOURCE_GAMES
uint8_t selectedGameIndex = 0;
GameId selectedGame = GameId::Catch1D;
#else
uint8_t selectedGameIndex = 1;
GameId selectedGame = GameId::ColourShooter;
#endif
State state = State::Selecting;
TwangGame twang;
Pong1DGame pong;
#if GAME_SET_SOURCE_GAMES
Catch1DGame catch1D;
ColourGateGame colourGate;
#else
ColourShooterGame colourShooter;
Tennis1DGame tennis;
ReactionRaceGame reactionRace;
ColourSnakeDuelGame colourSnakeDuel;
#endif
PowerStressTest powerStressTest;
uint32_t lastRenderAt = 0;
uint32_t modeButtonPressedAt = 0;
bool modeButtonLongHandled = false;
uint32_t powerChordStartedAt = 0;
uint32_t powerNoticeStartedAt = 0;
bool powerChordTracking = false;
bool powerChordHandled = false;
bool powerNoticeActive = false;
uint32_t stressChordStartedAt = 0;
bool stressChordTracking = false;
bool stressChordHandled = false;

#if ENABLE_SERIAL_DIAGNOSTICS
const __FlashStringHelper* gameName(GameId game) {
  switch (game) {
    case GameId::Twang:
      return F("Twang");
    case GameId::ColourShooter:
      return F("Colour Shooter");
    case GameId::Pong1D:
      return F("1D Pong");
    case GameId::Tennis1D:
      return F("Tennis 1D");
    case GameId::ReactionRace:
      return F("Reaction Race");
    case GameId::ColourSnakeDuel:
      return F("Colour Snake Duel");
    case GameId::MemorySequence:
      return F("Memory Sequence");
    case GameId::Snake1D:
      return F("Snake 1D");
    case GameId::Catch1D:
      return F("Catch 1D");
    case GameId::ColourGate:
      return F("Colour Gate");
    case GameId::Count:
      break;
  }

  return F("Unknown");
}
#endif

uint32_t gameColor(GameId game) {
  switch (game) {
    case GameId::Twang:
      return Config::MODE_TWANG_COLOR;
    case GameId::ColourShooter:
      return Config::MODE_COLOUR_SHOOTER_COLOR;
    case GameId::Pong1D:
      return Config::MODE_PONG_COLOR;
    case GameId::Tennis1D:
      return Config::MODE_TENNIS_COLOR;
    case GameId::ReactionRace:
      return Config::MODE_REACTION_RACE_COLOR;
    case GameId::ColourSnakeDuel:
      return Config::MODE_COLOUR_SNAKE_COLOR;
    case GameId::MemorySequence:
      return Config::MODE_MEMORY_COLOR;
    case GameId::Snake1D:
      return Config::MODE_SNAKE_COLOR;
    case GameId::Catch1D:
      return Config::MODE_CATCH_COLOR;
    case GameId::ColourGate:
      return Config::MODE_COLOUR_GATE_COLOR;
    case GameId::Count:
      break;
  }

  return 0;
}

void printSelection() {
  DEBUG_PRINT(F("Selected: "));
  DEBUG_PRINTLN(gameName(selectedGame));
}

void changeSelection(int8_t direction) {
  int8_t next = static_cast<int8_t>(selectedGameIndex) + direction;
  if (next < 0) {
    next = GAME_COUNT - 1;
  } else if (next >= GAME_COUNT) {
    next = 0;
  }
  selectedGameIndex = static_cast<uint8_t>(next);
  selectedGame = static_cast<GameId>(
      pgm_read_byte(&GAME_CATALOG[selectedGameIndex]));
  printSelection();
}

void startSelectedGame(uint32_t now) {
  state = State::Running;
  switch (selectedGame) {
    case GameId::Twang:
      twang.start(now);
      break;
#if !GAME_SET_SOURCE_GAMES
    case GameId::ColourShooter:
      colourShooter.start(now);
      break;
#endif
    case GameId::Pong1D:
      pong.start(now);
      break;
#if GAME_SET_SOURCE_GAMES
    case GameId::Catch1D:
      catch1D.start(now);
      break;
    case GameId::ColourGate:
      colourGate.start(now);
      break;
#else
    case GameId::Tennis1D:
      tennis.start(now);
      break;
    case GameId::ReactionRace:
      reactionRace.start(now);
      break;
    case GameId::ColourSnakeDuel:
      colourSnakeDuel.start(now);
      break;
#endif
    default:
      break;
  }
}

void returnToSelector() {
  state = State::Selecting;
  LedManager::clearStrip();
  DEBUG_PRINTLN(F("Returned to game selector"));
  printSelection();
}

bool powerModeChordHeld() {
  return InputManager::isHeld(InputManager::Button::Blue) &&
         InputManager::isHeld(InputManager::Button::Yellow) &&
         !InputManager::isHeld(InputManager::Button::Red);
}

void updatePowerModeChord(uint32_t now) {
  if (state != State::Selecting || !powerModeChordHeld()) {
    powerChordTracking = false;
    powerChordHandled = false;
    return;
  }

  if (!powerChordTracking) {
    powerChordTracking = true;
    powerChordStartedAt = now;
    DEBUG_PRINTLN(F("Hold Blue + Yellow to toggle power mode"));
    return;
  }

  if (!powerChordHandled &&
      static_cast<uint32_t>(now - powerChordStartedAt) >=
          Config::POWER_MODE_TOGGLE_HOLD_MS) {
    powerChordHandled = true;
    PowerModeManager::toggleMode();
    powerNoticeStartedAt = now;
    powerNoticeActive = true;
  }
}

bool powerStressChordHeld() {
  return InputManager::isHeld(InputManager::Button::Red) &&
         InputManager::isHeld(InputManager::Button::Blue) &&
         !InputManager::isHeld(InputManager::Button::Yellow);
}

void updatePowerStressChord(uint32_t now) {
  if (state != State::Selecting || !powerStressChordHeld()) {
    stressChordTracking = false;
    stressChordHandled = false;
    return;
  }

  if (!stressChordTracking) {
    stressChordTracking = true;
    stressChordStartedAt = now;
    DEBUG_PRINTLN(F("Hold Red + Blue to start power stress"));
    return;
  }

  if (!stressChordHandled &&
      static_cast<uint32_t>(now - stressChordStartedAt) >=
          Config::POWER_STRESS_HOLD_MS) {
    stressChordHandled = true;
    powerNoticeActive = false;
    state = State::PowerStress;
    powerStressTest.start(now);
  }
}

bool anyButtonPressed() {
  return InputManager::wasPressed(InputManager::Button::Red) ||
         InputManager::wasPressed(InputManager::Button::Green) ||
         InputManager::wasPressed(InputManager::Button::Blue) ||
         InputManager::wasPressed(InputManager::Button::Yellow) ||
         InputManager::wasPressed(InputManager::Button::EncoderClick) ||
         InputManager::wasPressed(InputManager::Button::Setup) ||
         InputManager::wasPressed(InputManager::Button::ModeSelect);
}

void render(uint32_t now) {
  if (state == State::Selecting) {
    LedManager::clearStrip();
    if (powerNoticeActive) {
      const uint32_t noticeColor =
          PowerModeManager::isPsuMode() ? Config::PSU_MODE_READY_COLOR
                                        : Config::BENCH_MODE_READY_COLOR;
      LedManager::setModePixel(noticeColor);
      for (uint8_t index = 0; index < 12; ++index) {
        LedManager::setStripPixel(index, noticeColor);
      }
    } else if (stressChordTracking && !stressChordHandled) {
      const uint32_t pulseColor =
          ((now / 150U) % 2U) == 0U ? Config::BUTTON_1_COLOR
                                     : Config::BUTTON_3_COLOR;
      LedManager::setModePixel(pulseColor);
      uint8_t progress = static_cast<uint8_t>(
          (static_cast<uint32_t>(now - stressChordStartedAt) * 12U) /
          Config::POWER_STRESS_HOLD_MS);
      if (progress > 12) {
        progress = 12;
      }
      for (uint8_t index = 0; index < progress; ++index) {
        LedManager::setStripPixel(
            index, (index & 0x01U) == 0U ? Config::BUTTON_1_COLOR
                                         : Config::BUTTON_3_COLOR);
      }
    } else if (powerChordTracking && !powerChordHandled) {
      LedManager::setModePixel(Config::PSU_MODE_ARMING_COLOR);
      uint8_t progress = static_cast<uint8_t>(
          (static_cast<uint32_t>(now - powerChordStartedAt) * 12U) /
          Config::POWER_MODE_TOGGLE_HOLD_MS);
      if (progress > 12) {
        progress = 12;
      }
      for (uint8_t index = 0; index < progress; ++index) {
        LedManager::setStripPixel(index, Config::PSU_MODE_ARMING_COLOR);
      }
    } else {
      LedManager::setModePixel(gameColor(selectedGame));
    }
    DisplayManager::showSelection(selectedGame);
  } else if (state == State::PowerStress) {
    powerStressTest.render();
  } else {
    switch (selectedGame) {
      case GameId::Twang:
        twang.render(now);
        DisplayManager::showSingleScore(twang.score());
        break;
#if !GAME_SET_SOURCE_GAMES
      case GameId::ColourShooter:
        colourShooter.render(now);
        DisplayManager::showSingleScore(colourShooter.score());
        break;
#endif
      case GameId::Pong1D:
        pong.render(now);
        DisplayManager::showVersusScore(pong.leftScore(), pong.rightScore());
        break;
#if GAME_SET_SOURCE_GAMES
      case GameId::Catch1D:
        catch1D.render(now);
        DisplayManager::showSingleScore(catch1D.score());
        break;
      case GameId::ColourGate:
        colourGate.render(now);
        DisplayManager::showSingleScore(colourGate.score());
        break;
#else
      case GameId::Tennis1D:
        tennis.render(now);
        DisplayManager::showVersusScore(tennis.player1Score(),
                                        tennis.player2Score());
        break;
      case GameId::ReactionRace:
        reactionRace.render(now);
        DisplayManager::showVersusScore(reactionRace.player1Score(),
                                        reactionRace.player2Score());
        break;
      case GameId::ColourSnakeDuel:
        colourSnakeDuel.render(now);
        DisplayManager::showVersusScore(colourSnakeDuel.player1Score(),
                                        colourSnakeDuel.player2Score());
        break;
#endif
      default:
        LedManager::clearStrip();
        break;
    }
    LedManager::setModePixel(gameColor(selectedGame));
  }

  LedManager::show();
}

}  // namespace

void begin(uint32_t now) {
  lastRenderAt = now - Config::RENDER_INTERVAL_MS;
  DEBUG_PRINTLN(F("Mode button: short press selects, long press confirms"));
  DEBUG_PRINTLN(F("Mode/setup button exits a running game"));
  DEBUG_PRINTLN(F("At selector: hold Blue + Yellow to toggle power mode"));
  DEBUG_PRINTLN(F("At selector: hold Red + Blue to start 10 s power stress"));
  printSelection();
}

void update(uint32_t now) {
  const bool modePressed =
      InputManager::wasPressed(InputManager::Button::ModeSelect);
  const bool modeReleased =
      InputManager::wasReleased(InputManager::Button::ModeSelect);

  if (powerNoticeActive &&
      static_cast<uint32_t>(now - powerNoticeStartedAt) >=
          Config::POWER_MODE_NOTICE_MS) {
    powerNoticeActive = false;
  }
  updatePowerModeChord(now);
  updatePowerStressChord(now);

  if (modePressed) {
    modeButtonPressedAt = now;
    modeButtonLongHandled = false;
  }

  bool modeLongPress = false;
  if (InputManager::isHeld(InputManager::Button::ModeSelect) &&
      !modeButtonLongHandled &&
      static_cast<uint32_t>(now - modeButtonPressedAt) >=
          Config::MODE_BUTTON_LONG_PRESS_MS) {
    modeButtonLongHandled = true;
    modeLongPress = true;
  }

  const bool modeShortPress = modeReleased && !modeButtonLongHandled;

  if (state == State::Selecting) {
    if (modeLongPress) {
      startSelectedGame(now);
    } else if (modeShortPress) {
      changeSelection(1);
    }
  } else if (state == State::PowerStress) {
    if (anyButtonPressed()) {
      if (modePressed) {
        modeButtonLongHandled = true;
      }
      powerStressTest.stop();
      returnToSelector();
    } else {
      powerStressTest.update(now);
      if (powerStressTest.isFinished()) {
        returnToSelector();
      }
    }
  } else {
    if (modePressed ||
        InputManager::wasPressed(InputManager::Button::Setup)) {
      if (modePressed) {
        modeButtonLongHandled = true;
      }
      returnToSelector();
    } else {
      switch (selectedGame) {
        case GameId::Twang:
          twang.update(now);
          break;
#if !GAME_SET_SOURCE_GAMES
        case GameId::ColourShooter:
          colourShooter.update(now);
          break;
#endif
        case GameId::Pong1D:
          pong.update(now);
          break;
#if GAME_SET_SOURCE_GAMES
        case GameId::Catch1D:
          catch1D.update(now);
          break;
        case GameId::ColourGate:
          colourGate.update(now);
          break;
#else
        case GameId::Tennis1D:
          tennis.update(now);
          break;
        case GameId::ReactionRace:
          reactionRace.update(now);
          break;
        case GameId::ColourSnakeDuel:
          colourSnakeDuel.update(now);
          break;
#endif
        default:
          break;
      }
    }
  }

  if (static_cast<uint32_t>(now - lastRenderAt) >=
      Config::RENDER_INTERVAL_MS) {
    lastRenderAt = now;
    render(now);
  }
}

}  // namespace GameManager
