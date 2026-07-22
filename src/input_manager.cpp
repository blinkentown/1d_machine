#include "input_manager.h"

#include <avr/pgmspace.h>

#include "config.h"
#include "pins.h"

static_assert(digitalPinToInterrupt(Pins::PLAYER_1_ENCODER_A) !=
                  NOT_AN_INTERRUPT,
              "Player 1 encoder A must support interrupts");
static_assert(digitalPinToInterrupt(Pins::PLAYER_1_ENCODER_B) !=
                  NOT_AN_INTERRUPT,
              "Player 1 encoder B must support interrupts");
static_assert(digitalPinToInterrupt(Pins::ENCODER_A) != NOT_AN_INTERRUPT,
              "Player 2 encoder A must support interrupts");
static_assert(digitalPinToInterrupt(Pins::ENCODER_B) != NOT_AN_INTERRUPT,
              "Player 2 encoder B must support interrupts");

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

DebouncedInput redButton(Pins::BUTTON_1, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput greenButton(Pins::BUTTON_2, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput blueButton(Pins::BUTTON_3, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput yellowButton(Pins::BUTTON_4, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput encoderClick(Pins::ENCODER_CLICK, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput setup(Pins::SETUP_BUTTON, Config::BUTTON_DEBOUNCE_MS);
DebouncedInput modeSelect(Pins::MODE_BUTTON, Config::BUTTON_DEBOUNCE_MS);

volatile uint8_t previousPlayer1EncoderState = 0;
volatile int8_t player1EncoderStepAccumulator = 0;
volatile int8_t pendingPlayer1EncoderDelta = 0;
volatile uint8_t previousPlayer2EncoderState = 0;
volatile int8_t player2EncoderStepAccumulator = 0;
volatile int8_t pendingPlayer2EncoderDelta = 0;
int8_t player1EncoderDelta = 0;
int8_t player2EncoderDelta = 0;

const int8_t ENCODER_TRANSITIONS[16] PROGMEM = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0,
};

DebouncedInput& inputFor(Button button) {
  switch (button) {
    case Button::Red:
      return redButton;
    case Button::Green:
      return greenButton;
    case Button::Blue:
      return blueButton;
    case Button::Yellow:
      return yellowButton;
    case Button::EncoderClick:
      return encoderClick;
    case Button::Setup:
      return setup;
    case Button::ModeSelect:
      return modeSelect;
  }

  return redButton;
}

void updatePlayer2EncoderInterrupt() {
  const uint8_t currentState =
      (static_cast<uint8_t>(digitalRead(Pins::ENCODER_A) == LOW) << 1U) |
      static_cast<uint8_t>(digitalRead(Pins::ENCODER_B) == LOW);

  if (currentState == previousPlayer2EncoderState) {
    return;
  }

  const uint8_t transition =
      (previousPlayer2EncoderState << 2U) | currentState;
  const int8_t step = static_cast<int8_t>(
      pgm_read_byte(&ENCODER_TRANSITIONS[transition]));
  previousPlayer2EncoderState = currentState;

  if (step == 0) {
    player2EncoderStepAccumulator = 0;
    return;
  }

  player2EncoderStepAccumulator += step * Config::ENCODER_DIRECTION;
  if (player2EncoderStepAccumulator >= Config::ENCODER_STEPS_PER_DETENT) {
    if (pendingPlayer2EncoderDelta < 127) {
      ++pendingPlayer2EncoderDelta;
    }
    player2EncoderStepAccumulator = 0;
  } else if (player2EncoderStepAccumulator <=
             -Config::ENCODER_STEPS_PER_DETENT) {
    if (pendingPlayer2EncoderDelta > -127) {
      --pendingPlayer2EncoderDelta;
    }
    player2EncoderStepAccumulator = 0;
  }
}

void updatePlayer1EncoderInterrupt() {
  const uint8_t currentState =
      (static_cast<uint8_t>(digitalRead(Pins::PLAYER_1_ENCODER_A) == LOW)
       << 1U) |
      static_cast<uint8_t>(digitalRead(Pins::PLAYER_1_ENCODER_B) == LOW);
  if (currentState == previousPlayer1EncoderState) {
    return;
  }

  const uint8_t transition =
      (previousPlayer1EncoderState << 2U) | currentState;
  const int8_t step = static_cast<int8_t>(
      pgm_read_byte(&ENCODER_TRANSITIONS[transition]));
  previousPlayer1EncoderState = currentState;
  if (step == 0) {
    player1EncoderStepAccumulator = 0;
    return;
  }

  player1EncoderStepAccumulator +=
      step * Config::PLAYER_1_ENCODER_DIRECTION;
  if (player1EncoderStepAccumulator >= Config::ENCODER_STEPS_PER_DETENT) {
    if (pendingPlayer1EncoderDelta < 127) {
      ++pendingPlayer1EncoderDelta;
    }
    player1EncoderStepAccumulator = 0;
  } else if (player1EncoderStepAccumulator <=
             -Config::ENCODER_STEPS_PER_DETENT) {
    if (pendingPlayer1EncoderDelta > -127) {
      --pendingPlayer1EncoderDelta;
    }
    player1EncoderStepAccumulator = 0;
  }
}

}  // namespace

void begin() {
  redButton.begin();
  greenButton.begin();
  blueButton.begin();
  yellowButton.begin();
  encoderClick.begin();
  setup.begin();
  modeSelect.begin();

  pinMode(Pins::PLAYER_1_ENCODER_A, INPUT_PULLUP);
  pinMode(Pins::PLAYER_1_ENCODER_B, INPUT_PULLUP);
  previousPlayer1EncoderState =
      (static_cast<uint8_t>(digitalRead(Pins::PLAYER_1_ENCODER_A) == LOW)
       << 1U) |
      static_cast<uint8_t>(digitalRead(Pins::PLAYER_1_ENCODER_B) == LOW);
  attachInterrupt(digitalPinToInterrupt(Pins::PLAYER_1_ENCODER_A),
                  updatePlayer1EncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Pins::PLAYER_1_ENCODER_B),
                  updatePlayer1EncoderInterrupt, CHANGE);

  pinMode(Pins::ENCODER_A, INPUT_PULLUP);
  pinMode(Pins::ENCODER_B, INPUT_PULLUP);
  previousPlayer2EncoderState =
      (static_cast<uint8_t>(digitalRead(Pins::ENCODER_A) == LOW) << 1U) |
      static_cast<uint8_t>(digitalRead(Pins::ENCODER_B) == LOW);
  attachInterrupt(digitalPinToInterrupt(Pins::ENCODER_A),
                  updatePlayer2EncoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Pins::ENCODER_B),
                  updatePlayer2EncoderInterrupt, CHANGE);
}

void update(uint32_t now) {
  noInterrupts();
  player1EncoderDelta = pendingPlayer1EncoderDelta;
  pendingPlayer1EncoderDelta = 0;
  player2EncoderDelta = pendingPlayer2EncoderDelta;
  pendingPlayer2EncoderDelta = 0;
  interrupts();

  redButton.update(now);
  greenButton.update(now);
  blueButton.update(now);
  yellowButton.update(now);
  encoderClick.update(now);
  setup.update(now);
  modeSelect.update(now);
}

bool isHeld(Button button) { return inputFor(button).isHeld(); }

bool wasPressed(Button button) { return inputFor(button).wasPressed(); }

bool wasReleased(Button button) { return inputFor(button).wasReleased(); }

int8_t encoderDelta(Encoder encoder) {
  return encoder == Encoder::Player1 ? player1EncoderDelta
                                     : player2EncoderDelta;
}

}  // namespace InputManager
