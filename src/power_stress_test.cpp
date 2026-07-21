#include "power_stress_test.h"

#include "config.h"
#include "led_manager.h"
#include "power_mode_manager.h"

void PowerStressTest::start(uint32_t now) {
  startedAt_ = now;
  active_ = true;
  Serial.print(F("Power stress started with "));
  Serial.print(PowerModeManager::isPsuMode() ? F("PSU") : F("BENCH"));
  Serial.println(F(" limits; press any button to stop"));
}

void PowerStressTest::update(uint32_t now) {
  if (active_ &&
      static_cast<uint32_t>(now - startedAt_) >=
          Config::POWER_STRESS_DURATION_MS) {
    active_ = false;
    Serial.println(F("Power stress completed"));
  }
}

void PowerStressTest::stop() {
  if (active_) {
    active_ = false;
    Serial.println(F("Power stress stopped by button"));
  }
}

void PowerStressTest::render() const {
  LedManager::clearStrip();
  if (!active_) {
    return;
  }

  for (uint16_t index = 0; index < Config::LED_COUNT; ++index) {
    LedManager::setStripPixel(index, Config::POWER_STRESS_COLOR);
  }
  LedManager::setModePixel(PowerModeManager::isPsuMode()
                               ? Config::PSU_MODE_READY_COLOR
                               : Config::BENCH_MODE_READY_COLOR);
}

bool PowerStressTest::isFinished() const { return !active_; }
