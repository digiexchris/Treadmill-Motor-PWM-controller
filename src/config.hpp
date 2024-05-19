#pragma once
#include <cstdint>
#include <sys/_stdint.h>
#include <zephyr/drivers/pwm.h>

const uint32_t PWM_FREQUENCY = PWM_HZ(40); // 40hz

#define ENABLE_DISPLAY 1

#define RPM_TIMER_INTERVAL_MSEC 250 // 250 milliseconds
#define RPM_STACK_SIZE 1024
#define RPM_THREAD_PRIORITY 7
#define RPM_COUNTER_TIME_BETWEEN_NUM_TICKS 5

#define ENCODER_MAX_VAL 360

//#define DEBUG_QDEC 1