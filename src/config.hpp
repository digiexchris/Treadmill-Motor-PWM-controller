#pragma once
#include <cstdint>
#include <lvgl.h>

const uint16_t PWM_FREQUENCY = 25000; // 40hz

#define RPM_TIMER_INTERVAL_MSEC 250 // 250 milliseconds
#define RPM_STACK_SIZE 1024
#define RPM_THREAD_PRIORITY 7

// // Some ready-made 16-bit ('565') color settings from the Adafruit ST77xx library:
// #define ST77XX_BLACK 0x0000
// #define ST77XX_WHITE 0xFFFF
// #define ST77XX_RED 0xF800
// #define ST77XX_GREEN 0x07E0
// #define ST77XX_BLUE 0x001F
// #define ST77XX_CYAN 0x07FF
// #define ST77XX_MAGENTA 0xF81F
// #define ST77XX_YELLOW 0xFFE0
// const lv_color16_t *ST77XX_ORANGE = {0xFC00