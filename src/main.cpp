#include <Arduino.h>

#include "config.h"
#include "game_manager.h"
#include "input_manager.h"
#include "led_manager.h"
#include "power_mode_manager.h"

namespace {

bool gameManagerStarted = false;

}  // namespace

void setup() {
  Serial.begin(Config::SERIAL_BAUD_RATE);
  InputManager::begin();
  LedManager::begin();
  const uint32_t now = millis();
  PowerModeManager::begin(now);

  Serial.println(F("1d_machine game firmware ready"));

  if (PowerModeManager::isReady()) {
    GameManager::begin(now);
    gameManagerStarted = true;
  }
}

void loop() {
  const uint32_t now = millis();
  InputManager::update(now);
  PowerModeManager::update(now);

  if (!PowerModeManager::isReady()) {
    return;
  }

  if (!gameManagerStarted) {
    GameManager::begin(now);
    gameManagerStarted = true;
    return;
  }

  GameManager::update(now);
}
