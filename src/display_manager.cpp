#include "display_manager.h"

#include <avr/pgmspace.h>

#include "config.h"
#include "pins.h"

namespace DisplayManager {
namespace {

constexpr uint8_t DIGIT_COUNT = 6;
constexpr uint8_t SEGMENT_DP = 0x80;

constexpr uint8_t GLYPH_A = 0x77;
constexpr uint8_t GLYPH_C = 0x39;
constexpr uint8_t GLYPH_G = 0x3D;
constexpr uint8_t GLYPH_H = 0x76;
constexpr uint8_t GLYPH_P = 0x73;
constexpr uint8_t GLYPH_S = 0x6D;
constexpr uint8_t GLYPH_N = 0x54;
constexpr uint8_t GLYPH_R = 0x50;
constexpr uint8_t GLYPH_T = 0x78;

const uint8_t DIGIT_GLYPHS[10] PROGMEM = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F,
};

const uint8_t MODE_GLYPHS[15] PROGMEM = {
    GLYPH_T, GLYPH_N, GLYPH_G,  // Twang: tNG
    GLYPH_C, GLYPH_S, GLYPH_H,  // Colour Shooter: CSH
    GLYPH_P, GLYPH_N, GLYPH_G,  // Pong: PnG
    GLYPH_R, GLYPH_A, GLYPH_C,  // Reaction Race: rAC
    GLYPH_C, GLYPH_S, GLYPH_N,  // Colour Snake Duel: CSn
};

uint8_t previousDigits[DIGIT_COUNT] = {};
bool displayInitialized = false;

void lineLow(uint8_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}

void lineHigh(uint8_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, INPUT);
}

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

uint8_t rotateGlyph180(uint8_t glyph) {
  uint8_t rotated = glyph & (SEGMENT_DP | 0x40U);
  rotated |= (glyph & 0x01U) << 3U;  // A -> D
  rotated |= (glyph & 0x02U) << 3U;  // B -> E
  rotated |= (glyph & 0x04U) << 3U;  // C -> F
  rotated |= (glyph & 0x08U) >> 3U;  // D -> A
  rotated |= (glyph & 0x10U) >> 3U;  // E -> B
  rotated |= (glyph & 0x20U) >> 3U;  // F -> C
  return rotated;
}

void mapDigitsToModule(const uint8_t* logical, uint8_t* physical) {
  if (Config::TM1637_ROTATE_180) {
    physical[0] = rotateGlyph180(logical[3]);
    physical[1] = rotateGlyph180(logical[4]);
    physical[2] = rotateGlyph180(logical[5]);
    physical[3] = rotateGlyph180(logical[0]);
    physical[4] = rotateGlyph180(logical[1]);
    physical[5] = rotateGlyph180(logical[2]);
    return;
  }

  // Common six-digit modules wire their grids as 3, 2, 1, 6, 5, 4.
  physical[0] = logical[2];
  physical[1] = logical[1];
  physical[2] = logical[0];
  physical[3] = logical[5];
  physical[4] = logical[4];
  physical[5] = logical[3];
}

void setModeGlyphs(uint8_t* digits, uint8_t start, Mode mode) {
  const uint8_t offset = static_cast<uint8_t>(mode) * 3U;
  for (uint8_t index = 0; index < 3U; ++index) {
    digits[start + index] = pgm_read_byte(&MODE_GLYPHS[offset + index]);
  }
}

void setScoreGlyphs(uint8_t* digits, uint8_t start, uint16_t score) {
  if (score > 999U) {
    score = 999U;
  }
  if (score >= 100U) {
    digits[start] = digitGlyph(static_cast<uint8_t>(score / 100U));
  }
  if (score >= 10U) {
    digits[start + 1U] =
        digitGlyph(static_cast<uint8_t>((score / 10U) % 10U));
  }
  digits[start + 2U] = digitGlyph(static_cast<uint8_t>(score % 10U));
}

void writeDisplay(const uint8_t* digits) {
  uint8_t mappedDigits[DIGIT_COUNT];
  mapDigitsToModule(digits, mappedDigits);
  bool changed = !displayInitialized;
  for (uint8_t index = 0; index < DIGIT_COUNT; ++index) {
    if (mappedDigits[index] != previousDigits[index]) {
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
    writeByte(mappedDigits[index]);
    previousDigits[index] = mappedDigits[index];
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
  const bool twoPlayers = mode == Mode::Pong1D ||
                          mode == Mode::ReactionRace ||
                          mode == Mode::ColourSnakeDuel;
  digits[0] = digitGlyph(twoPlayers ? 2U : 1U);
  digits[1] = GLYPH_P;
  setModeGlyphs(digits, 3U, mode);
  writeDisplay(digits);
}

void showSingleScore(uint16_t player1Score) {
  uint8_t digits[DIGIT_COUNT] = {};
  setScoreGlyphs(digits, 0U, player1Score);
  writeDisplay(digits);
}

void showVersusScore(uint16_t player1Score, uint16_t player2Score) {
  uint8_t digits[DIGIT_COUNT] = {};
  setScoreGlyphs(digits, 0U, player1Score);
  setScoreGlyphs(digits, 3U, player2Score);
  writeDisplay(digits);
}

}  // namespace DisplayManager
