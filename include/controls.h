#pragma once

#include "input_manager.h"

namespace Controls {

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

}  // namespace Controls
