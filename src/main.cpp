#include <Arduino.h>

#include "config.h"
#include "game_manager.h"
#include "input_manager.h"
#include "led_manager.h"

void setup() {
  Serial.begin(Config::SERIAL_BAUD_RATE);
  InputManager::begin();
  LedManager::begin();
  GameManager::begin(millis());

  Serial.println(F("1d_machine game firmware ready"));
}

void loop() {
  const uint32_t now = millis();
  InputManager::update(now);
  GameManager::update(now);
}
