#pragma once

#include <Arduino.h>

#ifndef ENABLE_SERIAL_DIAGNOSTICS
#define ENABLE_SERIAL_DIAGNOSTICS 0
#endif

#ifndef GAME_SET_SOURCE_GAMES
#define GAME_SET_SOURCE_GAMES 0
#endif

#if ENABLE_SERIAL_DIAGNOSTICS
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...) do { } while (0)
#define DEBUG_PRINTLN(...) do { } while (0)
#endif

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
constexpr uint8_t TM1637_BRIGHTNESS = 2;
constexpr uint8_t TM1637_HALF_CLOCK_US = 100;
constexpr bool TM1637_ROTATE_180 = true;

static_assert(TM1637_BRIGHTNESS <= 7, "TM1637 brightness must be 0..7");

constexpr uint16_t BUTTON_DEBOUNCE_MS = 20;
constexpr uint16_t MODE_BUTTON_LONG_PRESS_MS = 800;
constexpr uint16_t PSU_MODE_BOOT_HOLD_MS = 2000;
constexpr uint16_t POWER_MODE_TOGGLE_HOLD_MS = 2000;
constexpr uint16_t POWER_MODE_NOTICE_MS = 1000;
constexpr uint16_t POWER_STRESS_HOLD_MS = 2000;
constexpr int8_t PLAYER_1_ENCODER_DIRECTION = 1;
constexpr int8_t ENCODER_DIRECTION = -1;
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
constexpr uint32_t MODE_TENNIS_COLOR = 0x00FFFFUL;
constexpr uint32_t MODE_REACTION_RACE_COLOR = 0x00FF00UL;
constexpr uint32_t MODE_COLOUR_SNAKE_COLOR = 0x8000FFUL;
constexpr uint32_t MODE_SNAKE_COLOR = 0x00FFFFUL;
constexpr uint32_t MODE_MEMORY_COLOR = 0x8000FFUL;
constexpr uint32_t MODE_CATCH_COLOR = 0xFF00FFUL;
constexpr uint32_t MODE_COLOUR_GATE_COLOR = 0x0080FFUL;
constexpr uint32_t MODE_CODEBREAKER_COLOR = 0x8000FFUL;

constexpr uint16_t POWER_STRESS_DURATION_MS = 10000;
constexpr uint32_t POWER_STRESS_COLOR = 0xFFFFFFUL;
constexpr uint32_t PSU_MODE_READY_COLOR = 0xFF0000UL;
constexpr uint32_t PSU_MODE_ARMING_COLOR = 0xFFFF00UL;
constexpr uint32_t BENCH_MODE_READY_COLOR = 0x00FF00UL;

constexpr uint8_t PONG_PADDLE_LENGTH = 2;
constexpr uint8_t PONG_HIT_ZONE_LENGTH = PONG_PADDLE_LENGTH;
constexpr uint8_t PONG_MINIMUM_HIT_ZONE_PIXELS = GAME_PIXEL_WIDTH;
constexpr uint8_t PONG_ZONE_SHRINK_PIXELS_PER_POINT = 1;
constexpr uint8_t PONG_HIT_QUALITY_BANDS = 3;
constexpr uint16_t PONG_INITIAL_STEP_MS = 20;
constexpr uint16_t PONG_MINIMUM_SERVE_STEP_MS = 14;
constexpr uint16_t PONG_MINIMUM_STEP_MS = 8;
constexpr uint8_t PONG_SPEEDUP_MS = 2;
constexpr uint8_t PONG_SERVE_SPEEDUP_MS = 1;
constexpr uint16_t PONG_PERFECT_HIT_EXPLOSION_MS = 240;
constexpr uint8_t PONG_PERFECT_HIT_STROBE_MS = 30;
constexpr uint8_t PONG_PERFECT_HIT_RADIUS = EXPLOSION_INTENSITY * 4U;
constexpr uint16_t PONG_POINT_DELAY_MS = 1000;
constexpr uint8_t PONG_WINNING_SCORE = 5;
constexpr uint32_t PONG_BALL_COLOR = 0xFFFFFFUL;
constexpr uint32_t PONG_LEFT_PLAYER_COLOR = BUTTON_1_COLOR;
constexpr uint32_t PONG_RIGHT_PLAYER_COLOR = BUTTON_3_COLOR;
static_assert(PONG_MINIMUM_HIT_ZONE_PIXELS > 0,
              "Pong minimum hit zone must be visible");
static_assert(PONG_MINIMUM_HIT_ZONE_PIXELS <=
                  PONG_HIT_ZONE_LENGTH * GAME_PIXEL_WIDTH,
              "Pong minimum hit zone cannot exceed its starting width");

constexpr uint8_t TENNIS_COURT_LENGTH = LED_COUNT / 4U;
constexpr uint8_t TENNIS_PADDLE_WIDTH = GAME_PIXEL_WIDTH;
constexpr uint8_t TENNIS_HIT_EDGE_WIDTH = TAPE_PIXEL_WIDTH;
constexpr uint8_t TENNIS_ENCODER_PIXELS_PER_DETENT = 1;
constexpr uint8_t TENNIS_LANDING_FRONT_MARGIN = GAME_PIXEL_WIDTH;
constexpr uint8_t TENNIS_LANDING_DEPTH_STEP = 15;
constexpr uint16_t TENNIS_FLIGHT_BASE_MS = 1050;
constexpr uint8_t TENNIS_FLIGHT_SPEEDUP_MS = 130;
constexpr uint8_t TENNIS_BOUNCE_BASE_STEP_MS = 22;
constexpr uint8_t TENNIS_BOUNCE_SPEEDUP_MS = 4;
constexpr uint16_t TENNIS_FAST_DETENT_MS = 35;
constexpr uint16_t TENNIS_MEDIUM_DETENT_MS = 75;
constexpr uint16_t TENNIS_SWING_MEMORY_MS = 140;
constexpr uint16_t TENNIS_SERVE_DELAY_MS = 900;
constexpr uint16_t TENNIS_POINT_DELAY_MS = 900;
constexpr uint16_t TENNIS_HIT_EFFECT_MS = 180;
constexpr uint8_t TENNIS_WINNING_SCORE = 5;
constexpr uint32_t TENNIS_PLAYER_1_COLOR = 0xFF0000UL;
constexpr uint32_t TENNIS_PLAYER_2_COLOR = 0x0000FFUL;
constexpr uint32_t TENNIS_BALL_COLOR = 0xFFFFFFUL;
constexpr uint32_t TENNIS_ARC_COLOR = 0x004040UL;
constexpr uint32_t TENNIS_LANDING_MARKER_COLOR = 0x002020UL;
static_assert(LED_COUNT % 4U == 0U,
              "Tennis requires four equal court quarters");
static_assert(TENNIS_PADDLE_WIDTH <= TENNIS_COURT_LENGTH,
              "Tennis paddle must fit inside one player court");
static_assert(TENNIS_LANDING_FRONT_MARGIN +
                      3U * TENNIS_LANDING_DEPTH_STEP <
                  TENNIS_COURT_LENGTH,
              "Tennis deepest landing must remain inside the court");
static_assert(TENNIS_BOUNCE_BASE_STEP_MS >
                  4U * TENNIS_BOUNCE_SPEEDUP_MS,
              "Tennis bounce interval must remain positive");

constexpr uint8_t TWANG_STARTING_LIVES = 3;
constexpr uint8_t TWANG_START_CELL = 1;
constexpr uint8_t TWANG_ATTACK_RANGE_CELLS = 3;
constexpr uint8_t TWANG_JUMP_CELLS = 2;
constexpr uint8_t TWANG_PIXEL_STEP_MS = 12;
constexpr uint16_t TWANG_EFFECT_MS = 300;
constexpr uint16_t TWANG_LEVEL_CLEAR_MS = 900;
constexpr uint32_t TWANG_PLAYER_COLOR = 0xFFFFFFUL;
constexpr uint32_t TWANG_ATTACK_COLOR = 0x0080FFUL;
constexpr uint32_t TWANG_ENEMY_COLOR = 0xFF0000UL;
constexpr uint32_t TWANG_LAVA_COLOR = 0xFF4000UL;
constexpr uint32_t TWANG_EXIT_COLOR = 0x00FF00UL;
constexpr uint32_t TWANG_LIFE_COLOR = 0x004000UL;

constexpr uint8_t REACTION_ROUNDS_TO_WIN = 3;
constexpr uint16_t REACTION_WAIT_MIN_MS = 1500;
constexpr uint16_t REACTION_WAIT_MAX_MS = 4000;
constexpr uint16_t REACTION_ROUND_RESULT_MS = 1000;
constexpr uint8_t REACTION_PIXEL_STEP_MS = 12;
constexpr uint32_t REACTION_PLAYER_1_COLOR = 0xFF0000UL;
constexpr uint32_t REACTION_PLAYER_2_COLOR = 0x0000FFUL;
constexpr uint32_t REACTION_WAIT_COLOR = 0xFF8000UL;
constexpr uint32_t REACTION_GO_COLOR = 0x00FF00UL;
constexpr uint32_t REACTION_FALSE_START_COLOR = 0xFFFF00UL;

constexpr uint8_t METEOR_STARTING_LIVES = 3;
constexpr uint8_t METEOR_STARTING_SHIELDS = 3;
constexpr uint8_t METEOR_BLAST_RADIUS_CELLS = 2;
constexpr uint16_t METEOR_INITIAL_WARNING_MS = 1200;
constexpr uint16_t METEOR_MINIMUM_WARNING_MS = 450;
constexpr uint8_t METEOR_WARNING_SPEEDUP_MS = 35;
constexpr uint8_t METEOR_MAX_SPEED_STEPS =
    (METEOR_INITIAL_WARNING_MS - METEOR_MINIMUM_WARNING_MS) /
    METEOR_WARNING_SPEEDUP_MS;
constexpr uint16_t METEOR_IMPACT_MS = 360;
constexpr uint32_t METEOR_PLAYER_COLOR = 0xFFFFFFUL;
constexpr uint32_t METEOR_WARNING_COLOR = 0xFF8000UL;
constexpr uint32_t METEOR_BLAST_COLOR = 0xFF0000UL;
constexpr uint32_t METEOR_SHIELD_COLOR = 0x00FFFFUL;
constexpr uint32_t METEOR_LIFE_COLOR = 0x004000UL;
static_assert(METEOR_INITIAL_WARNING_MS -
                      METEOR_MAX_SPEED_STEPS * METEOR_WARNING_SPEEDUP_MS >=
                  METEOR_MINIMUM_WARNING_MS,
              "Meteor warning speed must respect its minimum");

constexpr uint8_t MEMORY_START_LENGTH = 1;
constexpr uint8_t MEMORY_MAX_LENGTH = 32;
constexpr uint16_t MEMORY_SHOW_ON_MS = 400;
constexpr uint16_t MEMORY_SHOW_GAP_MS = 180;
constexpr uint16_t MEMORY_INPUT_FEEDBACK_MS = 180;
constexpr uint16_t MEMORY_INPUT_TIMEOUT_MS = 3500;
constexpr uint16_t MEMORY_SUCCESS_MS = 700;
constexpr uint32_t MEMORY_SUCCESS_COLOR = 0x00FF00UL;
constexpr uint32_t MEMORY_ERROR_COLOR = 0xFF0000UL;

constexpr uint8_t CATCH_INITIAL_TARGET_WIDTH = GAME_PIXEL_WIDTH * 3U;
constexpr uint8_t CATCH_MINIMUM_TARGET_WIDTH = GAME_PIXEL_WIDTH;
constexpr uint8_t CATCH_TARGET_SHRINK_PIXELS_PER_HIT = 2;
constexpr uint8_t CATCH_MARKER_WIDTH = TAPE_PIXEL_WIDTH;
constexpr uint8_t CATCH_INITIAL_STEP_MS = 14;
constexpr uint8_t CATCH_MINIMUM_STEP_MS = 6;
constexpr uint8_t CATCH_SPEEDUP_MS = 1;
constexpr uint8_t CATCH_MAX_CATCHUP_STEPS = 12;
constexpr uint16_t CATCH_HIT_EFFECT_MS = 300;
constexpr uint32_t CATCH_TARGET_COLOR = 0x004000UL;
constexpr uint32_t CATCH_MARKER_COLOR = 0xFFFFFFUL;
constexpr uint32_t CATCH_SUCCESS_COLOR = 0x00FF00UL;
constexpr uint32_t CATCH_ERROR_COLOR = 0xFF0000UL;
static_assert(CATCH_MINIMUM_TARGET_WIDTH > 0,
              "Catch minimum target must be visible");
static_assert(CATCH_MINIMUM_TARGET_WIDTH <= CATCH_INITIAL_TARGET_WIDTH,
              "Catch minimum target cannot exceed its starting width");
static_assert(CATCH_INITIAL_TARGET_WIDTH <= LED_COUNT,
              "Catch target must fit on the strip");

constexpr uint8_t COLOUR_GATE_STARTING_LIVES = 3;
constexpr uint16_t COLOUR_GATE_CENTER = LED_COUNT / 4U;
constexpr uint8_t COLOUR_GATE_WIDTH = GAME_PIXEL_WIDTH * 2U;
constexpr uint8_t COLOUR_GATE_CUE_WIDTH = TAPE_PIXEL_WIDTH;
constexpr uint8_t COLOUR_GATE_INITIAL_STEP_MS = 10;
constexpr uint8_t COLOUR_GATE_MINIMUM_STEP_MS = 5;
constexpr uint8_t COLOUR_GATE_SPEEDUP_MS = 1;
constexpr uint8_t COLOUR_GATE_SPEEDUP_EVERY = 3;
constexpr uint8_t COLOUR_GATE_STAGE_SCORE = 8;
constexpr uint8_t COLOUR_GATE_MAX_CATCHUP_STEPS = 12;
constexpr uint16_t COLOUR_GATE_FEEDBACK_MS = 320;
constexpr uint32_t COLOUR_GATE_COLOR = 0x202020UL;
constexpr uint32_t COLOUR_GATE_SUCCESS_COLOR = 0x00FF00UL;
constexpr uint32_t COLOUR_GATE_ERROR_COLOR = 0xFF0000UL;
constexpr uint32_t COLOUR_GATE_LIFE_COLOR = 0x004000UL;
constexpr uint16_t COLOUR_QUEST_TRANSITION_MS = 800;
static_assert(COLOUR_GATE_WIDTH > 0 &&
                  COLOUR_GATE_CENTER >= COLOUR_GATE_WIDTH / 2U &&
                  COLOUR_GATE_CENTER + COLOUR_GATE_WIDTH / 2U < LED_COUNT,
              "Colour Gate target must fit on the strip");

constexpr uint8_t BOSS_DEFLECT_STARTING_LIVES = 3;
constexpr uint16_t BOSS_DEFLECT_GATE_CENTER = LED_COUNT / 4U;
constexpr uint8_t BOSS_DEFLECT_GATE_WIDTH = GAME_PIXEL_WIDTH * 2U;
constexpr uint8_t BOSS_DEFLECT_BOSS_WIDTH = GAME_PIXEL_WIDTH * 2U;
constexpr uint8_t BOSS_DEFLECT_ATTACK_WIDTH = TAPE_PIXEL_WIDTH;
constexpr uint8_t BOSS_DEFLECT_BASE_HEALTH = 4;
constexpr uint8_t BOSS_DEFLECT_MAX_HEALTH = 12;
constexpr uint8_t BOSS_DEFLECT_INITIAL_STEP_MS = 12;
constexpr uint8_t BOSS_DEFLECT_MAX_LEVEL_SPEEDUP = 6;
constexpr uint8_t BOSS_DEFLECT_REFLECTED_STEP_MS = 4;
constexpr uint8_t BOSS_DEFLECT_MAX_CATCHUP_STEPS = 12;
constexpr uint16_t BOSS_DEFLECT_FEEDBACK_MS = 280;
constexpr uint16_t BOSS_DEFLECT_LEVEL_CLEAR_MS = 800;
constexpr uint32_t BOSS_DEFLECT_GATE_COLOR = 0x202020UL;
constexpr uint32_t BOSS_DEFLECT_BOSS_COLOR = 0xFF2000UL;
constexpr uint32_t BOSS_DEFLECT_SUCCESS_COLOR = 0x00FF00UL;
constexpr uint32_t BOSS_DEFLECT_ERROR_COLOR = 0xFF0000UL;
constexpr uint32_t BOSS_DEFLECT_LIFE_COLOR = 0x004000UL;
static_assert(BOSS_DEFLECT_INITIAL_STEP_MS >
                  BOSS_DEFLECT_MAX_LEVEL_SPEEDUP,
              "Boss incoming step interval must stay positive");
static_assert(BOSS_DEFLECT_BOSS_WIDTH < LED_COUNT / 2U,
              "Boss must leave room for the playfield");

constexpr uint8_t CODEBREAKER_CODE_LENGTH = 4;
constexpr uint8_t CODEBREAKER_ATTEMPTS = 8;
constexpr uint16_t CODEBREAKER_SUCCESS_MS = 800;
constexpr uint32_t CODEBREAKER_SLOT_COLOR = 0x101010UL;
constexpr uint32_t CODEBREAKER_EXACT_COLOR = 0x00FF00UL;
constexpr uint32_t CODEBREAKER_MISPLACED_COLOR = 0xFF8000UL;
constexpr uint32_t CODEBREAKER_MISS_COLOR = 0x300000UL;
constexpr uint32_t CODEBREAKER_ATTEMPT_COLOR = 0x004000UL;
static_assert(CODEBREAKER_CODE_LENGTH <= 8,
              "Codebreaker secret must fit in its packed state");

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

constexpr uint8_t COLOUR_SNAKE_INITIAL_LENGTH = 3;
constexpr uint8_t COLOUR_SNAKE_ROUNDS_TO_WIN = 5;
constexpr uint8_t COLOUR_SNAKE_INITIAL_PIXEL_STEP_MS = 90;
constexpr uint8_t COLOUR_SNAKE_MINIMUM_PIXEL_STEP_MS = 38;
constexpr uint16_t COLOUR_SNAKE_SPEEDUP_EVERY_MS = 4000;
constexpr uint8_t COLOUR_SNAKE_SPEEDUP_MS = 4;
constexpr uint8_t COLOUR_SNAKE_MAX_SPEED_STEPS =
    (COLOUR_SNAKE_INITIAL_PIXEL_STEP_MS -
     COLOUR_SNAKE_MINIMUM_PIXEL_STEP_MS) /
    COLOUR_SNAKE_SPEEDUP_MS;
constexpr uint8_t COLOUR_SNAKE_SHOT_STEP_MS = 6;
constexpr uint8_t COLOUR_SNAKE_PENALTY_STEP_MS = 25;
constexpr uint16_t COLOUR_SNAKE_HIT_EFFECT_MS = 260;
constexpr uint16_t COLOUR_SNAKE_ROUND_RESULT_MS = 900;
constexpr uint32_t COLOUR_SNAKE_ERROR_COLOR = 0xFF0000UL;
static_assert(COLOUR_SNAKE_INITIAL_LENGTH <
                  LED_COUNT / GAME_PIXEL_WIDTH / 2U,
              "Colour Snake must start before either player endpoint");
static_assert(COLOUR_SNAKE_INITIAL_PIXEL_STEP_MS -
                      COLOUR_SNAKE_MAX_SPEED_STEPS *
                          COLOUR_SNAKE_SPEEDUP_MS >=
                  COLOUR_SNAKE_MINIMUM_PIXEL_STEP_MS,
              "Colour Snake growth speed must respect its minimum");

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
