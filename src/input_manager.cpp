#include "input_manager.h"

#include <avr/pgmspace.h>

#include "config.h"
#include "pins.h"

namespace InputManager {
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

  bool isHeld() const { return stableState_ == LOW; }
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

DebouncedInput game1(Pins::BUTTON_1, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput game2(Pins::BUTTON_2, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput game3(Pins::BUTTON_3, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput game4(Pins::BUTTON_4, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput encoderClick(Pins::ENCODER_CLICK, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput setup(Pins::SETUP_BUTTON, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput modeSelect(Pins::MODE_BUTTON, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput encoderA(Pins::ENCODER_A, Config::ENCODER_DEBOUNCE_MS);
DebouncedInput encoderB(Pins::ENCODER_B, Config::ENCODER_DEBOUNCE_MS);

uint8_t previousEncoderState = 0;
int8_t encoderStepAccumulator = 0;
int8_t pendingEncoderDelta = 0;

const int8_t ENCODER_TRANSITIONS[16] PROGMEM = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0,
};

DebouncedInput& inputFor(Button button) {
  switch (button) {
    case Button::Game1:
      return game1;
    case Button::Game2:
      return game2;
    case Button::Game3:
      return game3;
    case Button::Game4:
      return game4;
    case Button::EncoderClick:
      return encoderClick;
    case Button::Setup:
      return setup;
    case Button::ModeSelect:
      return modeSelect;
  }

  return game1;
}

void updateEncoder() {
  const uint8_t currentState =
      (static_cast<uint8_t>(encoderA.isHeld()) << 1) |
      static_cast<uint8_t>(encoderB.isHeld());

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
    pendingEncoderDelta = 1;
    encoderStepAccumulator = 0;
  } else if (encoderStepAccumulator <= -Config::ENCODER_STEPS_PER_DETENT) {
    pendingEncoderDelta = -1;
    encoderStepAccumulator = 0;
  }
}

}  // namespace

void begin() {
  game1.begin();
  game2.begin();
  game3.begin();
  game4.begin();
  encoderClick.begin();
  setup.begin();
  modeSelect.begin();
  encoderA.begin();
  encoderB.begin();

  previousEncoderState =
      (static_cast<uint8_t>(encoderA.isHeld()) << 1) |
      static_cast<uint8_t>(encoderB.isHeld());
}

void update(uint32_t now) {
  game1.update(now);
  game2.update(now);
  game3.update(now);
  game4.update(now);
  encoderClick.update(now);
  setup.update(now);
  modeSelect.update(now);
  encoderA.update(now);
  encoderB.update(now);
  updateEncoder();
}

bool isHeld(Button button) { return inputFor(button).isHeld(); }

bool wasPressed(Button button) { return inputFor(button).wasPressed(); }

bool wasReleased(Button button) { return inputFor(button).wasReleased(); }

int8_t takeEncoderDelta() {
  const int8_t result = pendingEncoderDelta;
  pendingEncoderDelta = 0;
  return result;
}

}  // namespace InputManager
