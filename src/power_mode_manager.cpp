#include "power_mode_manager.h"

#include "config.h"
#include "input_manager.h"
#include "led_manager.h"

namespace PowerModeManager {
namespace {

enum class StartupState : uint8_t {
  ArmingPsu,
  WaitingForRelease,
  Ready,
};

StartupState startupState = StartupState::Ready;
bool psuMode = false;
uint32_t armStartedAt = 0;

bool bootChordHeld() {
  return InputManager::isHeld(InputManager::Button::Game3) &&
         InputManager::isHeld(InputManager::Button::Game4);
}

void applyBenchMode() {
  psuMode = false;
  LedManager::setPowerLimits(Config::BENCH_LED_BRIGHTNESS,
                             Config::BENCH_LED_MAX_MILLIAMPS);
  LedManager::clearStrip();
  LedManager::setModePixel(0);
  LedManager::show();
  Serial.println(F("Power mode: BENCH"));
}

void applyPsuMode() {
  psuMode = true;
  LedManager::setPowerLimits(Config::PSU_LED_BRIGHTNESS,
                             Config::PSU_LED_MAX_MILLIAMPS);
  LedManager::clearStrip();
  LedManager::setModePixel(Config::POWER_TEST_READY_COLOR);
  LedManager::show();
  Serial.println(F("Power mode: PSU (latched until reset)"));
}

}  // namespace

void begin(uint32_t now) {
  applyBenchMode();

  if (bootChordHeld()) {
    startupState = StartupState::ArmingPsu;
    armStartedAt = now;
    LedManager::setModePixel(Config::PSU_MODE_ARMING_COLOR);
    LedManager::show();
    Serial.println(F("Hold Blue + Yellow for 2 seconds to arm PSU mode"));
  } else {
    startupState = StartupState::Ready;
  }
}

void update(uint32_t now) {
  if (startupState == StartupState::Ready) {
    return;
  }

  if (startupState == StartupState::WaitingForRelease) {
    if (!InputManager::isHeld(InputManager::Button::Game3) &&
        !InputManager::isHeld(InputManager::Button::Game4)) {
      startupState = StartupState::Ready;
    }
    return;
  }

  if (!bootChordHeld()) {
    startupState = StartupState::WaitingForRelease;
    applyBenchMode();
    Serial.println(F("PSU mode arming cancelled"));
    return;
  }

  if (static_cast<uint32_t>(now - armStartedAt) >=
      Config::PSU_MODE_BOOT_HOLD_MS) {
    startupState = StartupState::WaitingForRelease;
    applyPsuMode();
  }
}

bool isReady() { return startupState == StartupState::Ready; }

bool isPsuMode() { return psuMode; }

}  // namespace PowerModeManager
