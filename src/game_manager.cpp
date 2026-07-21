#include "game_manager.h"

#include "config.h"
#include "games/colour_shooter.h"
#include "games/pong_1d.h"
#include "games/snake_1d.h"
#include "games/twang.h"
#include "input_manager.h"
#include "led_manager.h"
#include "power_mode_manager.h"
#include "power_stress_test.h"

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
  PowerStress,
  Running,
};

GameId selectedGame = GameId::ColourShooter;
State state = State::Selecting;
TwangGame twang;
ColourShooterGame colourShooter;
Pong1DGame pong;
Snake1DGame snake;
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

bool selectedGameIsImplemented() {
  return selectedGame == GameId::Twang ||
         selectedGame == GameId::ColourShooter ||
         selectedGame == GameId::Pong1D || selectedGame == GameId::Snake1D;
}

void startSelectedGame(uint32_t now) {
  if (!selectedGameIsImplemented()) {
    Serial.print(gameName(selectedGame));
    Serial.println(F(" is not implemented yet"));
    return;
  }

  state = State::Running;
  switch (selectedGame) {
    case GameId::Twang:
      twang.start(now);
      break;
    case GameId::ColourShooter:
      colourShooter.start(now);
      break;
    case GameId::Pong1D:
      pong.start(now);
      break;
    case GameId::Snake1D:
      snake.start(now);
      break;
    default:
      break;
  }
}

void returnToSelector() {
  state = State::Selecting;
  LedManager::clearStrip();
  Serial.println(F("Returned to game selector"));
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
    Serial.println(F("Hold Red + Blue to start power stress"));
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
  } else if (state == State::PowerStress) {
    powerStressTest.render();
  } else {
    switch (selectedGame) {
      case GameId::Twang:
        twang.render(now);
        break;
      case GameId::ColourShooter:
        colourShooter.render(now);
        break;
      case GameId::Pong1D:
        pong.render(now);
        break;
      case GameId::Snake1D:
        snake.render(now);
        break;
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
  Serial.println(F("Mode button: short press selects, long press confirms"));
  Serial.println(F("Mode/setup button exits a running game"));
  Serial.println(F("At selector: hold Blue + Yellow to toggle power mode"));
  Serial.println(F("At selector: hold Red + Blue to start 10 s power stress"));
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
    } else if (encoderDelta != 0) {
      changeSelection(encoderDelta > 0 ? 1 : -1);
    }

    if (InputManager::wasPressed(InputManager::Button::EncoderClick)) {
      startSelectedGame(now);
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
        case GameId::ColourShooter:
          colourShooter.update(now);
          break;
        case GameId::Pong1D:
          pong.update(now);
          break;
        case GameId::Snake1D:
          snake.update(now);
          break;
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
