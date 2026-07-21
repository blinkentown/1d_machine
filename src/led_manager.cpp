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
  FastLED.setBrightness(Config::LED_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(Config::LED_SUPPLY_VOLTS,
                                        Config::LED_MAX_MILLIAMPS);
  FastLED.clear(true);
}

void clearStrip() {
  fill_solid(stripLeds, Config::LED_COUNT, CRGB::Black);
}

void setStripPixel(uint16_t index, uint32_t color) {
  if (index < Config::LED_COUNT) {
    stripLeds[index] = CRGB(color);
  }
}

void setModePixel(uint32_t color) { modePixel[0] = CRGB(color); }

void show() { FastLED.show(); }

}  // namespace LedManager
