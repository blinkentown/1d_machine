#include "power_test.h"

#include "config.h"
#include "led_manager.h"

void PowerTest::start(uint32_t now) {
  stage_ = Stage::DarkBaseline;
  stageStartedAt_ = now;
  Serial.println(F("Power preflight: dark baseline"));
  Serial.println(F("Software-limited preview only; no current sensor is fitted"));
}

void PowerTest::advance(uint32_t now) {
  stageStartedAt_ = now;

  switch (stage_) {
    case Stage::DarkBaseline:
      stage_ = Stage::SinglePixel;
      Serial.println(F("Power preflight: one white strip pixel"));
      break;
    case Stage::SinglePixel:
      stage_ = Stage::GameLoad;
      Serial.println(F("Power preflight: five-pixel worst-case game preview"));
      break;
    case Stage::GameLoad:
      stage_ = Stage::Ready;
      Serial.println(F("Power preview complete"));
      Serial.println(F("Press encoder to confirm and start the game"));
      break;
    case Stage::Ready:
      break;
  }
}

void PowerTest::update(uint32_t now) {
  if (stage_ != Stage::Ready &&
      static_cast<uint32_t>(now - stageStartedAt_) >=
          Config::POWER_TEST_STAGE_MS) {
    advance(now);
  }
}

void PowerTest::render() const {
  LedManager::clearStrip();

  switch (stage_) {
    case Stage::DarkBaseline:
      LedManager::setModePixel(0);
      break;
    case Stage::SinglePixel:
      LedManager::setStripPixel(Config::LED_COUNT / 2,
                                Config::POWER_TEST_COLOR);
      LedManager::setModePixel(Config::POWER_TEST_COLOR);
      break;
    case Stage::GameLoad:
      LedManager::setStripPixel(0, Config::POWER_TEST_COLOR);
      LedManager::setStripPixel(1, Config::POWER_TEST_COLOR);
      LedManager::setStripPixel(Config::LED_COUNT / 2,
                                Config::POWER_TEST_COLOR);
      LedManager::setStripPixel(Config::LED_COUNT - 2,
                                Config::POWER_TEST_COLOR);
      LedManager::setStripPixel(Config::LED_COUNT - 1,
                                Config::POWER_TEST_COLOR);
      LedManager::setModePixel(Config::POWER_TEST_COLOR);
      break;
    case Stage::Ready:
      LedManager::setModePixel(Config::POWER_TEST_READY_COLOR);
      break;
  }
}

bool PowerTest::isReady() const { return stage_ == Stage::Ready; }
