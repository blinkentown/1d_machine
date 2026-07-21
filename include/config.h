#pragma once

#include <Arduino.h>

namespace Config {

constexpr uint32_t SERIAL_BAUD_RATE = 115200UL;

constexpr uint16_t LED_COUNT = 288;
constexpr uint8_t TAPE_PIXEL_WIDTH = 3;
constexpr uint8_t GAME_SEGMENT_WIDTH_MULTIPLIER = 4;
constexpr uint8_t GAME_PIXEL_WIDTH =
    TAPE_PIXEL_WIDTH * GAME_SEGMENT_WIDTH_MULTIPLIER;
constexpr uint8_t EXPLOSION_INTENSITY = GAME_PIXEL_WIDTH;
constexpr uint8_t GAMEPLAY_SPEED_PERCENT = 100;
constexpr uint8_t SNAKE_SPEED_PERCENT = 80;

constexpr uint16_t gameplayInterval(uint16_t intervalMs,
                                    uint8_t gameSpeedPercent = 100) {
  return static_cast<uint16_t>(
      (static_cast<uint32_t>(intervalMs) * 10000UL) /
      (static_cast<uint16_t>(GAMEPLAY_SPEED_PERCENT) * gameSpeedPercent));
}

static_assert(GAMEPLAY_SPEED_PERCENT > 0, "Gameplay speed must be non-zero");
static_assert(SNAKE_SPEED_PERCENT > 0, "Snake speed must be non-zero");
constexpr uint8_t LED_SUPPLY_VOLTS = 5;
constexpr uint8_t BENCH_LED_BRIGHTNESS = 32;
constexpr uint16_t BENCH_LED_MAX_MILLIAMPS = 100;
constexpr uint8_t PSU_LED_BRIGHTNESS = 85;
constexpr uint16_t PSU_LED_MAX_MILLIAMPS = 3000;
constexpr uint16_t RENDER_INTERVAL_MS = 20;

constexpr uint16_t BUTTON_DEBOUNCE_MS = 20;
constexpr uint16_t MODE_BUTTON_LONG_PRESS_MS = 800;
constexpr uint16_t PSU_MODE_BOOT_HOLD_MS = 2000;
constexpr uint16_t POWER_MODE_TOGGLE_HOLD_MS = 2000;
constexpr uint16_t POWER_MODE_NOTICE_MS = 1000;
constexpr uint16_t POWER_STRESS_HOLD_MS = 2000;
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

constexpr uint16_t POWER_STRESS_DURATION_MS = 10000;
constexpr uint32_t POWER_STRESS_COLOR = 0xFFFFFFUL;
constexpr uint32_t PSU_MODE_READY_COLOR = 0xFF0000UL;
constexpr uint32_t PSU_MODE_ARMING_COLOR = 0xFFFF00UL;
constexpr uint32_t BENCH_MODE_READY_COLOR = 0x00FF00UL;

constexpr uint8_t PONG_PADDLE_LENGTH = 2;
constexpr uint8_t PONG_HIT_ZONE_LENGTH = PONG_PADDLE_LENGTH;
constexpr uint8_t PONG_HIT_QUALITY_BANDS = 3;
constexpr uint16_t PONG_INITIAL_STEP_MS = 20;
constexpr uint16_t PONG_MINIMUM_STEP_MS = 8;
constexpr uint8_t PONG_SPEEDUP_MS = 1;
constexpr uint16_t PONG_PERFECT_HIT_EXPLOSION_MS = 240;
constexpr uint8_t PONG_PERFECT_HIT_STROBE_MS = 30;
constexpr uint8_t PONG_PERFECT_HIT_RADIUS = EXPLOSION_INTENSITY * 4U;
constexpr uint16_t PONG_POINT_DELAY_MS = 1000;
constexpr uint8_t PONG_WINNING_SCORE = 5;
constexpr uint32_t PONG_BALL_COLOR = 0xFFFFFFUL;
constexpr uint32_t PONG_LEFT_PLAYER_COLOR = BUTTON_1_COLOR;
constexpr uint32_t PONG_RIGHT_PLAYER_COLOR = BUTTON_3_COLOR;
static_assert((PONG_HIT_ZONE_LENGTH * GAME_PIXEL_WIDTH) %
                      PONG_HIT_QUALITY_BANDS ==
                  0,
              "Pong hit zone must divide into equal quality bands");

constexpr uint8_t COLOUR_SHOOTER_STARTING_LIVES = 3;
constexpr uint8_t COLOUR_SHOOTER_TARGET_COUNT = 8;
constexpr uint8_t COLOUR_SHOOTER_MAX_SHOTS = 4;
constexpr uint8_t COLOUR_SHOOTER_MAX_DISSOLVES = 4;
constexpr uint16_t COLOUR_SHOOTER_FIRST_TARGET_POSITION = 85;
constexpr uint8_t COLOUR_SHOOTER_TARGET_SPACING = 27;
constexpr uint8_t COLOUR_SHOOTER_HOME_POSITION = 4;
constexpr uint16_t COLOUR_SHOOTER_INITIAL_STEP_MS = 65;
constexpr uint16_t COLOUR_SHOOTER_MINIMUM_STEP_MS = 25;
constexpr uint8_t COLOUR_SHOOTER_SPEEDUP_MS = 2;
constexpr uint8_t COLOUR_SHOOTER_SPEEDUP_EVERY = 4;
constexpr uint8_t COLOUR_SHOOTER_SHOT_STEP_MS = 6;
constexpr uint16_t COLOUR_SHOOTER_SPAWN_INTERVAL_MS = 500;
constexpr uint16_t COLOUR_SHOOTER_DISSOLVE_MS = 560;
constexpr uint16_t COLOUR_SHOOTER_STROBE_MS = 200;
constexpr uint8_t COLOUR_SHOOTER_STROBE_PERIOD_MS = 25;
constexpr uint8_t COLOUR_SHOOTER_BLAST_RADIUS =
    EXPLOSION_INTENSITY * 7U + 1U;
constexpr uint32_t COLOUR_SHOOTER_ERROR_COLOR = 0xFF0000UL;

constexpr uint8_t SNAKE_STARTING_LIVES = 3;
constexpr uint8_t SNAKE_MAX_SEGMENTS = 48;
constexpr uint8_t SNAKE_MAX_SHOTS = 4;
constexpr uint8_t SNAKE_MAX_BLASTS = 4;
constexpr uint8_t SNAKE_INITIAL_LENGTH_CELLS = 12;
constexpr uint8_t SNAKE_SPECIAL_WIDTH_CELLS = 3;
constexpr uint8_t SNAKE_SPECIAL_HITS = 3;
constexpr uint8_t SNAKE_SPECIAL_CHANCE = 7;
constexpr uint8_t SNAKE_HOME_POSITION = 4;
constexpr uint16_t SNAKE_INITIAL_STEP_MS = 72;
constexpr uint16_t SNAKE_MINIMUM_STEP_MS = 26;
constexpr uint8_t SNAKE_SPEEDUP_MS = 2;
constexpr uint8_t SNAKE_SPEEDUP_EVERY = 4;
constexpr uint8_t SNAKE_SHOT_STEP_MS = 6;
constexpr uint8_t SNAKE_SLOW_ZONE_CELLS = 5;
constexpr uint8_t SNAKE_CLOSE_SLOWDOWN_PERCENT = 30;
constexpr uint8_t SNAKE_BREACH_PUSHBACK_CELLS = 2;
constexpr uint16_t SNAKE_BLAST_MS = 420;
constexpr uint32_t SNAKE_ERROR_COLOR = 0xFF0000UL;

}  // namespace Config
