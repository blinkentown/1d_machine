#include "game_manager.h"

#include "config.h"
#include "games/colour_shooter.h"
#include "games/pong_1d.h"
#include "input_manager.h"
#include "led_manager.h"
#include "power_mode_manager.h"
#include "power_test.h"

namespace GameManager {
namespace {

enum class GameId : uint8_t {
  Twang,
  ColourShooter,
  Pong1D,
  ReactionRace,
  Snake1D,
  MeteorDodge,
  MemorySequence,
  Count,
};

enum class State : uint8_t {
  Selecting,
  PowerCheck,
  Running,
};

GameId selectedGame = GameId::ColourShooter;
State state = State::Selecting;
ColourShooterGame colourShooter;
Pong1DGame pong;
PowerTest powerTest;
uint32_t lastRenderAt = 0;
uint32_t modeButtonPressedAt = 0;
bool modeButtonLongHandled = false;
uint32_t powerChordStartedAt = 0;
uint32_t powerNoticeStartedAt = 0;
bool powerChordTracking = false;
bool powerChordHandled = false;
bool powerNoticeActive = false;

const __FlashStringHelper* gameName(GameId game) {
  switch (game) {
    case GameId::Twang:
      return F("Twang");
    case GameId::ColourShooter:
      return F("Colour Shooter");
    case GameId::Pong1D:
      return F("1D Pong");
    case GameId::ReactionRace:
      return F("Reaction Race");
    case GameId::Snake1D:
      return F("Snake 1D");
    case GameId::MeteorDodge:
      return F("Meteor Dodge");
    case GameId::MemorySequence:
      return F("Memory Sequence");
    case GameId::Count:
      break;
  }

  return F("Unknown");
}

uint32_t gameColor(GameId game) {
  switch (game) {
    case GameId::Twang:
      return Config::MODE_TWANG_COLOR;
    case GameId::ColourShooter:
      return Config::MODE_COLOUR_SHOOTER_COLOR;
    case GameId::Pong1D:
      return Config::MODE_PONG_COLOR;
    case GameId::ReactionRace:
      return Config::MODE_REACTION_RACE_COLOR;
    case GameId::Snake1D:
      return Config::MODE_SNAKE_COLOR;
    case GameId::MeteorDodge:
      return Config::MODE_METEOR_DODGE_COLOR;
    case GameId::MemorySequence:
      return Config::MODE_MEMORY_COLOR;
    case GameId::Count:
      break;
  }

  return 0;
}

void printSelection() {
  Serial.print(F("Selected: "));
  Serial.println(gameName(selectedGame));
}

void changeSelection(int8_t direction) {
  const int8_t gameCount = static_cast<int8_t>(GameId::Count);
  int8_t next = static_cast<int8_t>(selectedGame) + direction;
  if (next < 0) {
    next = gameCount - 1;
  } else if (next >= gameCount) {
    next = 0;
  }
  selectedGame = static_cast<GameId>(next);
  printSelection();
}

void beginPowerCheck(uint32_t now) {
  if (selectedGame != GameId::ColourShooter &&
      selectedGame != GameId::Pong1D) {
    Serial.print(gameName(selectedGame));
    Serial.println(F(" is not implemented yet"));
    return;
  }

  state = State::PowerCheck;
  powerTest.start(now);
}

void startSelectedGame(uint32_t now) {
  state = State::Running;
  if (selectedGame == GameId::ColourShooter) {
    colourShooter.start(now);
  } else {
    pong.start(now);
  }
}

void returnToSelector() {
  state = State::Selecting;
  LedManager::clearStrip();
  Serial.println(F("Returned to game selector"));
  printSelection();
}

bool powerModeChordHeld() {
  return InputManager::isHeld(InputManager::Button::Game3) &&
         InputManager::isHeld(InputManager::Button::Game4);
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
    Serial.println(F("Hold Blue + Yellow to toggle power mode"));
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

void render(uint32_t now) {
  if (state == State::Selecting) {
    LedManager::clearStrip();
    if (powerNoticeActive) {
      const uint32_t noticeColor =
          PowerModeManager::isPsuMode() ? Config::POWER_TEST_READY_COLOR
                                        : Config::BENCH_MODE_READY_COLOR;
      LedManager::setModePixel(noticeColor);
      for (uint8_t index = 0; index < 12; ++index) {
        LedManager::setStripPixel(index, noticeColor);
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
  } else if (state == State::PowerCheck) {
    powerTest.render();
  } else {
    if (selectedGame == GameId::ColourShooter) {
      colourShooter.render(now);
    } else {
      pong.render(now);
    }
    LedManager::setModePixel(gameColor(selectedGame));
  }

  LedManager::show();
}

}  // namespace

void begin(uint32_t now) {
  lastRenderAt = now - Config::RENDER_INTERVAL_MS;
  Serial.println(F("Mode button: short press selects, long press confirms"));
  Serial.println(F("Mode/setup button exits a running game"));
  Serial.println(F("At selector: hold Blue + Yellow to toggle power mode"));
  printSelection();
}

void update(uint32_t now) {
  const int8_t encoderDelta = InputManager::takeEncoderDelta();
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
      beginPowerCheck(now);
    } else if (modeShortPress) {
      changeSelection(1);
    } else if (encoderDelta != 0) {
      changeSelection(encoderDelta > 0 ? 1 : -1);
    }

    if (InputManager::wasPressed(InputManager::Button::EncoderClick)) {
      beginPowerCheck(now);
    }
  } else if (state == State::PowerCheck) {
    if (InputManager::wasPressed(InputManager::Button::Setup) ||
        modeShortPress) {
      returnToSelector();
    } else {
      powerTest.update(now);
      if (powerTest.isReady() &&
          (modeLongPress ||
           InputManager::wasPressed(InputManager::Button::EncoderClick))) {
        startSelectedGame(now);
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
      if (selectedGame == GameId::ColourShooter) {
        colourShooter.update(now);
      } else {
        pong.update(now);
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
