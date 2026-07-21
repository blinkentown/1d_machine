#pragma once

#include <Arduino.h>

namespace Config {

constexpr uint32_t SERIAL_BAUD_RATE = 115200UL;

constexpr uint16_t LED_COUNT = 288;
constexpr uint8_t LED_SUPPLY_VOLTS = 5;
constexpr uint8_t BENCH_LED_BRIGHTNESS = 32;
constexpr uint16_t BENCH_LED_MAX_MILLIAMPS = 100;
constexpr uint8_t PSU_LED_BRIGHTNESS = 16;
constexpr uint16_t PSU_LED_MAX_MILLIAMPS = 6000;
constexpr uint16_t RENDER_INTERVAL_MS = 20;

constexpr uint16_t BUTTON_DEBOUNCE_MS = 20;
constexpr uint16_t MODE_BUTTON_LONG_PRESS_MS = 800;
constexpr uint16_t PSU_MODE_BOOT_HOLD_MS = 2000;
constexpr uint16_t ENCODER_DEBOUNCE_MS = 2;
constexpr int8_t ENCODER_DIRECTION = 1;
constexpr int8_t ENCODER_STEPS_PER_DETENT = 4;

constexpr uint32_t BUTTON_1_COLOR = 0xFF0000UL;
constexpr uint32_t BUTTON_2_COLOR = 0x00FF00UL;
constexpr uint32_t BUTTON_3_COLOR = 0x0000FFUL;
constexpr uint32_t BUTTON_4_COLOR = 0xFFFF00UL;
constexpr uint32_t SETUP_BUTTON_COLOR = 0xFFFF00UL;
constexpr uint32_t ENCODER_CLICK_COLOR = 0x8000FFUL;

constexpr uint32_t MODE_TWANG_COLOR = 0xFF4000UL;
constexpr uint32_t MODE_COLOUR_SHOOTER_COLOR = 0xFFFF00UL;
constexpr uint32_t MODE_PONG_COLOR = 0x0000FFUL;
constexpr uint32_t MODE_REACTION_RACE_COLOR = 0x00FF00UL;
constexpr uint32_t MODE_SNAKE_COLOR = 0x00FFFFUL;
constexpr uint32_t MODE_METEOR_DODGE_COLOR = 0xFF0000UL;
constexpr uint32_t MODE_MEMORY_COLOR = 0x8000FFUL;

constexpr uint16_t POWER_TEST_STAGE_MS = 1200;
constexpr uint16_t POWER_TEST_FULL_LOAD_MS = 10000;
constexpr uint32_t POWER_TEST_COLOR = 0xFFFFFFUL;
constexpr uint32_t POWER_TEST_READY_COLOR = 0x00FF00UL;
constexpr uint32_t PSU_MODE_ARMING_COLOR = 0xFFFF00UL;

constexpr uint8_t PONG_PADDLE_LENGTH = 2;
constexpr uint8_t PONG_HIT_ZONE_LENGTH = 12;
constexpr uint16_t PONG_INITIAL_STEP_MS = 20;
constexpr uint16_t PONG_MINIMUM_STEP_MS = 8;
constexpr uint8_t PONG_SPEEDUP_MS = 1;
constexpr uint16_t PONG_POINT_PAUSE_MS = 1000;
constexpr uint8_t PONG_WINNING_SCORE = 5;
constexpr uint32_t PONG_BALL_COLOR = 0xFFFFFFUL;
constexpr uint32_t PONG_LEFT_PLAYER_COLOR = BUTTON_1_COLOR;
constexpr uint32_t PONG_RIGHT_PLAYER_COLOR = BUTTON_3_COLOR;

constexpr uint8_t COLOUR_SHOOTER_STARTING_LIVES = 3;
constexpr uint16_t COLOUR_SHOOTER_INITIAL_STEP_MS = 10;
constexpr uint16_t COLOUR_SHOOTER_MINIMUM_STEP_MS = 3;
constexpr uint8_t COLOUR_SHOOTER_SPEEDUP_MS = 1;
constexpr uint16_t COLOUR_SHOOTER_CORRECT_FEEDBACK_MS = 150;
constexpr uint16_t COLOUR_SHOOTER_ERROR_FEEDBACK_MS = 600;
constexpr uint32_t COLOUR_SHOOTER_ERROR_COLOR = 0xFF0000UL;

}  // namespace Config
