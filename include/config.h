#pragma once

#include <Arduino.h>

namespace Config {

constexpr uint32_t SERIAL_BAUD_RATE = 115200UL;

constexpr uint16_t LED_COUNT = 288;
constexpr uint8_t LED_BRIGHTNESS = 85;
constexpr uint8_t LED_SUPPLY_VOLTS = 5;
constexpr uint16_t LED_MAX_MILLIAMPS = 6000;
constexpr uint16_t LED_FRAME_INTERVAL_MS = 20;

constexpr uint16_t BUTTON_DEBOUNCE_MS = 20;
constexpr uint16_t ENCODER_DEBOUNCE_MS = 2;
constexpr int8_t ENCODER_DIRECTION = 1;
constexpr int8_t ENCODER_STEPS_PER_DETENT = 4;

constexpr uint32_t BUTTON_1_COLOR = 0xFF0000UL;
constexpr uint32_t BUTTON_2_COLOR = 0x00FF00UL;
constexpr uint32_t BUTTON_3_COLOR = 0x0000FFUL;
constexpr uint32_t BUTTON_4_COLOR = 0xFFFFFFUL;
constexpr uint32_t SETUP_BUTTON_COLOR = 0xFFFF00UL;
constexpr uint32_t ENCODER_CLICK_COLOR = 0x8000FFUL;
constexpr uint32_t IDLE_PIXEL_COLOR = 0x2040FFUL;

}  // namespace Config
