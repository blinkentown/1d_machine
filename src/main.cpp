#include <Arduino.h>
#include <FastLED.h>
#include <avr/pgmspace.h>

#include "config.h"
#include "pins.h"

static_assert(Pins::LED_DATA == MOSI, "LED data must use the hardware-SPI MOSI pin");
static_assert(Pins::LED_CLOCK == SCK, "LED clock must use the hardware-SPI SCK pin");
static_assert(Config::TEST_LED_INDEX < Config::LED_COUNT,
              "Test LED index must be inside the strip");

namespace {

class DebouncedInput {
 public:
  DebouncedInput(uint8_t pin, uint16_t debounceMs)
      : pin_(pin), debounceMs_(debounceMs) {}

  void begin() {
    pinMode(pin_, INPUT_PULLUP);
    rawState_ = digitalRead(pin_);
    stableState_ = rawState_;
    rawChangedAt_ = millis();
  }

  void update(uint32_t now) {
    pressedEvent_ = false;
    releasedEvent_ = false;

    const bool sample = digitalRead(pin_);
    if (sample != rawState_) {
      rawState_ = sample;
      rawChangedAt_ = now;
    }

    if (rawState_ != stableState_ &&
        static_cast<uint32_t>(now - rawChangedAt_) >= debounceMs_) {
      stableState_ = rawState_;
      pressedEvent_ = stableState_ == LOW;
      releasedEvent_ = stableState_ == HIGH;
    }
  }

  bool isPressed() const { return stableState_ == LOW; }
  bool wasPressed() const { return pressedEvent_; }
  bool wasReleased() const { return releasedEvent_; }

 private:
  const uint8_t pin_;
  const uint16_t debounceMs_;
  bool rawState_ = HIGH;
  bool stableState_ = HIGH;
  bool pressedEvent_ = false;
  bool releasedEvent_ = false;
  uint32_t rawChangedAt_ = 0;
};

CRGB leds[Config::LED_COUNT];

DebouncedInput button1(Pins::BUTTON_1, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput button2(Pins::BUTTON_2, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput button3(Pins::BUTTON_3, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput button4(Pins::BUTTON_4, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput setupButton(Pins::SETUP_BUTTON, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput encoderClick(Pins::ENCODER_CLICK, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput encoderA(Pins::ENCODER_A, Config::ENCODER_DEBOUNCE_MS);
DebouncedInput encoderB(Pins::ENCODER_B, Config::ENCODER_DEBOUNCE_MS);

uint8_t previousEncoderState = 0;
int8_t encoderStepAccumulator = 0;
int32_t encoderPosition = 0;
uint32_t lastLedFrameAt = 0;

const int8_t ENCODER_TRANSITIONS[16] PROGMEM = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0,
};

void beginInputs() {
  button1.begin();
  button2.begin();
  button3.begin();
  button4.begin();
  setupButton.begin();
  encoderClick.begin();
  encoderA.begin();
  encoderB.begin();

  previousEncoderState =
      (static_cast<uint8_t>(encoderA.isPressed()) << 1) |
      static_cast<uint8_t>(encoderB.isPressed());
}

void updateInputs(uint32_t now) {
  button1.update(now);
  button2.update(now);
  button3.update(now);
  button4.update(now);
  setupButton.update(now);
  encoderClick.update(now);
  encoderA.update(now);
  encoderB.update(now);
}

void reportButton(const DebouncedInput& input,
                  const __FlashStringHelper* name) {
  if (!input.wasPressed() && !input.wasReleased()) {
    return;
  }

  Serial.print(name);
  Serial.println(input.wasPressed() ? F(": pressed") : F(": released"));
}

void reportButtonEvents() {
  reportButton(button1, F("Button 1"));
  reportButton(button2, F("Button 2"));
  reportButton(button3, F("Button 3"));
  reportButton(button4, F("Button 4"));
  reportButton(setupButton, F("Setup button"));
  reportButton(encoderClick, F("Encoder click"));
}

void updateEncoder() {
  const uint8_t currentState =
      (static_cast<uint8_t>(encoderA.isPressed()) << 1) |
      static_cast<uint8_t>(encoderB.isPressed());

  if (currentState == previousEncoderState) {
    return;
  }

  const uint8_t transition = (previousEncoderState << 2) | currentState;
  const int8_t step = static_cast<int8_t>(
      pgm_read_byte(&ENCODER_TRANSITIONS[transition]));
  previousEncoderState = currentState;

  if (step == 0) {
    encoderStepAccumulator = 0;
    return;
  }

  encoderStepAccumulator += step * Config::ENCODER_DIRECTION;
  if (encoderStepAccumulator >= Config::ENCODER_STEPS_PER_DETENT) {
    ++encoderPosition;
    encoderStepAccumulator = 0;
    Serial.print(F("Encoder: clockwise, position "));
    Serial.println(encoderPosition);
  } else if (encoderStepAccumulator <= -Config::ENCODER_STEPS_PER_DETENT) {
    --encoderPosition;
    encoderStepAccumulator = 0;
    Serial.print(F("Encoder: counter-clockwise, position "));
    Serial.println(encoderPosition);
  }
}

bool selectedButtonColor(CRGB& color) {
  if (button4.isPressed()) {
    color = CRGB(Config::BUTTON_4_COLOR);
  } else if (button3.isPressed()) {
    color = CRGB(Config::BUTTON_3_COLOR);
  } else if (button2.isPressed()) {
    color = CRGB(Config::BUTTON_2_COLOR);
  } else if (button1.isPressed()) {
    color = CRGB(Config::BUTTON_1_COLOR);
  } else {
    return false;
  }

  return true;
}

void updateLedTest(uint32_t now) {
  if (static_cast<uint32_t>(now - lastLedFrameAt) <
      Config::LED_FRAME_INTERVAL_MS) {
    return;
  }
  lastLedFrameAt = now;

  CRGB activeColor(Config::IDLE_PIXEL_COLOR);
  selectedButtonColor(activeColor);

  fill_solid(leds, Config::LED_COUNT, CRGB::Black);
  leds[Config::TEST_LED_INDEX] = activeColor;

  FastLED.show();
}

}  // namespace

void setup() {
  Serial.begin(Config::SERIAL_BAUD_RATE);
  beginInputs();

  FastLED.addLeds<APA102, Pins::LED_DATA, Pins::LED_CLOCK, BGR>(
      leds, Config::LED_COUNT);
  FastLED.setBrightness(Config::LED_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(Config::LED_SUPPLY_VOLTS,
                                        Config::LED_MAX_MILLIAMPS);
  FastLED.clear(true);

  Serial.println(F("1d_machine hardware test ready"));
}

void loop() {
  const uint32_t now = millis();
  updateInputs(now);
  reportButtonEvents();
  updateEncoder();
  updateLedTest(now);
}
