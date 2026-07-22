#pragma once

#include "input_manager.h"

namespace Controls {

enum class Player : uint8_t {
  One,
  Two,
};

constexpr InputManager::Button PLAYER_1_PRIMARY = InputManager::Button::Red;
constexpr InputManager::Button PLAYER_1_SECONDARY =
    InputManager::Button::Green;
constexpr InputManager::Button PLAYER_2_PRIMARY = InputManager::Button::Blue;
constexpr InputManager::Button PLAYER_2_SECONDARY =
    InputManager::Button::Yellow;

constexpr InputManager::Button ONE_PLAYER_LEFT = InputManager::Button::Red;
constexpr InputManager::Button ONE_PLAYER_RIGHT = InputManager::Button::Green;
constexpr InputManager::Button ONE_PLAYER_ACTION = InputManager::Button::Blue;
constexpr InputManager::Button ONE_PLAYER_SPECIAL =
    InputManager::Button::Yellow;

inline int8_t rotation(Player player) {
  return InputManager::encoderDelta(
      player == Player::One ? InputManager::Encoder::Player1
                            : InputManager::Encoder::Player2);
}

inline bool primaryPressed(Player player) {
  return InputManager::wasPressed(player == Player::One
                                      ? PLAYER_1_PRIMARY
                                      : PLAYER_2_PRIMARY);
}

inline bool secondaryPressed(Player player) {
  return InputManager::wasPressed(player == Player::One
                                      ? PLAYER_1_SECONDARY
                                      : PLAYER_2_SECONDARY);
}

}  // namespace Controls
