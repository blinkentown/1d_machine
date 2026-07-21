#include "display_manager.h"

#include <avr/pgmspace.h>

#include "config.h"
#include "pins.h"

namespace DisplayManager {
namespace {

constexpr uint8_t DIGIT_COUNT = 6;
constexpr uint8_t SEGMENT_DP = 0x80;

constexpr uint8_t GLYPH_BLANK = 0x00;
constexpr uint8_t GLYPH_C = 0x39;
constexpr uint8_t GLYPH_E = 0x79;
constexpr uint8_t GLYPH_G = 0x3D;
constexpr uint8_t GLYPH_M = 0x37;
constexpr uint8_t GLYPH_P = 0x73;
constexpr uint8_t GLYPH_S = 0x6D;
constexpr uint8_t GLYPH_N = 0x54;
constexpr uint8_t GLYPH_R = 0x50;
constexpr uint8_t GLYPH_T = 0x78;

const uint8_t DIGIT_GLYPHS[10] PROGMEM = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F,
};

const uint8_t MODE_GLYPHS[14] PROGMEM = {
    GLYPH_T, GLYPH_G,  // Twang
    GLYPH_C, GLYPH_S,  // Colour Shooter
    GLYPH_P, GLYPH_G,  // Pong
    GLYPH_R, GLYPH_C,  // Reaction Race
    GLYPH_S, GLYPH_N,  // Snake
    GLYPH_M, GLYPH_T,  // Meteor Dodge
    GLYPH_M, GLYPH_E,  // Memory Sequence
};

uint8_t previousDigits[DIGIT_COUNT] = {};
bool displayInitialized = false;

void lineLow(uint8_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

void lineHigh(uint8_t pin) { pinMode(pin, INPUT_PULLUP); }

void busDelay() { delayMicroseconds(Config::TM1637_HALF_CLOCK_US); }

void startCondition() {
  lineHigh(Pins::DISPLAY_DIO);
  lineHigh(Pins::DISPLAY_CLK);
  busDelay();
  lineLow(Pins::DISPLAY_DIO);
  busDelay();
  lineLow(Pins::DISPLAY_CLK);
}

void stopCondition() {
  lineLow(Pins::DISPLAY_DIO);
  busDelay();
  lineHigh(Pins::DISPLAY_CLK);
  busDelay();
  lineHigh(Pins::DISPLAY_DIO);
  busDelay();
}

void writeByte(uint8_t value) {
  for (uint8_t bit = 0; bit < 8; ++bit) {
    lineLow(Pins::DISPLAY_CLK);
    if ((value & 0x01U) != 0U) {
      lineHigh(Pins::DISPLAY_DIO);
    } else {
      lineLow(Pins::DISPLAY_DIO);
    }
    busDelay();
    lineHigh(Pins::DISPLAY_CLK);
    busDelay();
    value >>= 1;
  }

  lineLow(Pins::DISPLAY_CLK);
  lineHigh(Pins::DISPLAY_DIO);
  busDelay();
  lineHigh(Pins::DISPLAY_CLK);
  busDelay();
  lineLow(Pins::DISPLAY_CLK);
}

uint8_t digitGlyph(uint8_t digit) {
  return pgm_read_byte(&DIGIT_GLYPHS[digit % 10U]);
}

void setModeGlyphs(uint8_t* digits, Mode mode) {
  const uint8_t offset = static_cast<uint8_t>(mode) * 2U;
  digits[0] = pgm_read_byte(&MODE_GLYPHS[offset]);
  digits[1] = pgm_read_byte(&MODE_GLYPHS[offset + 1U]);
}

void writeDisplay(const uint8_t* digits) {
  bool changed = !displayInitialized;
  for (uint8_t index = 0; index < DIGIT_COUNT; ++index) {
    if (digits[index] != previousDigits[index]) {
      changed = true;
    }
  }
  if (!changed) {
    return;
  }

  startCondition();
  writeByte(0x40);
  stopCondition();

  startCondition();
  writeByte(0xC0);
  for (uint8_t index = 0; index < DIGIT_COUNT; ++index) {
    writeByte(digits[index]);
    previousDigits[index] = digits[index];
  }
  stopCondition();

  startCondition();
  writeByte(static_cast<uint8_t>(0x88U | Config::TM1637_BRIGHTNESS));
  stopCondition();
  displayInitialized = true;
}

}  // namespace

void begin() {
  lineHigh(Pins::DISPLAY_CLK);
  lineHigh(Pins::DISPLAY_DIO);
  displayInitialized = false;
  clear();
}

void clear() {
  const uint8_t digits[DIGIT_COUNT] = {};
  writeDisplay(digits);
}

void showSelection(Mode mode) {
  uint8_t digits[DIGIT_COUNT] = {};
  setModeGlyphs(digits, mode);
  writeDisplay(digits);
}

void showSingleScore(Mode mode, uint16_t score, uint8_t indicators) {
  uint8_t digits[DIGIT_COUNT] = {};
  setModeGlyphs(digits, mode);
  if (score > 9999U) {
    score = 9999U;
  }
  digits[2] = digitGlyph(static_cast<uint8_t>((score / 1000U) % 10U));
  digits[3] = digitGlyph(static_cast<uint8_t>((score / 100U) % 10U));
  digits[4] = digitGlyph(static_cast<uint8_t>((score / 10U) % 10U));
  digits[5] = digitGlyph(static_cast<uint8_t>(score % 10U));

  if (indicators > 3U) {
    indicators = 3U;
  }
  for (uint8_t index = 0; index < indicators; ++index) {
    digits[5U - index] |= SEGMENT_DP;
  }
  writeDisplay(digits);
}

void showVersusScore(Mode mode, uint8_t leftScore, uint8_t rightScore) {
  uint8_t digits[DIGIT_COUNT] = {};
  setModeGlyphs(digits, mode);
  if (leftScore > 99U) {
    leftScore = 99U;
  }
  if (rightScore > 99U) {
    rightScore = 99U;
  }
  digits[2] = leftScore >= 10U ? digitGlyph(leftScore / 10U) : GLYPH_BLANK;
  digits[3] = digitGlyph(leftScore % 10U) | SEGMENT_DP;
  digits[4] = rightScore >= 10U ? digitGlyph(rightScore / 10U) : GLYPH_BLANK;
  digits[5] = digitGlyph(rightScore % 10U);
  writeDisplay(digits);
}

}  // namespace DisplayManager
