#include "led_manager.h"

#include <FastLED.h>

#include "config.h"
#include "pins.h"

static_assert(Pins::LED_DATA == MOSI,
              "LED data must use the hardware-SPI MOSI pin");
static_assert(Pins::LED_CLOCK == SCK,
              "LED clock must use the hardware-SPI SCK pin");

namespace LedManager {
namespace {

CRGB stripLeds[Config::LED_COUNT];
CRGB modePixel[1];

}  // namespace

void begin() {
  FastLED.addLeds<APA102, Pins::LED_DATA, Pins::LED_CLOCK, BGR>(
      stripLeds, Config::LED_COUNT);
  FastLED.addLeds<NEOPIXEL, Pins::MODE_PIXEL_DATA>(modePixel, 1);
  setPowerLimits(Config::BENCH_LED_BRIGHTNESS,
                 Config::BENCH_LED_MAX_MILLIAMPS);
  FastLED.clear(true);
}

void setPowerLimits(uint8_t brightness, uint16_t maxMilliamps) {
  FastLED.setBrightness(brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(Config::LED_SUPPLY_VOLTS,
                                        maxMilliamps);
}

void clearStrip() {
  fill_solid(stripLeds, Config::LED_COUNT, CRGB::Black);
}

void setStripPixel(uint16_t index, uint32_t color) {
  if (index < Config::LED_COUNT) {
    stripLeds[index] = CRGB(color);
  }
}

void setStripRange(uint16_t start, uint16_t count, uint32_t color) {
  const uint16_t end =
      start + count > Config::LED_COUNT ? Config::LED_COUNT : start + count;
  for (uint16_t index = start; index < end; ++index) {
    stripLeds[index] = CRGB(color);
  }
}

void setGameCell(uint8_t cell, uint32_t color) {
  setStripRange(static_cast<uint16_t>(cell) * Config::GAME_PIXEL_WIDTH,
                Config::GAME_PIXEL_WIDTH, color);
}

void setModePixel(uint32_t color) { modePixel[0] = CRGB(color); }

void show() { FastLED.show(); }

}  // namespace LedManager
