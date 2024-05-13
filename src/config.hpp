#pragma once
#include <cstdint>
#include <sys/_stdint.h>

const uint16_t PWM_FREQUENCY = 25000; // 40hz

#define RPM_TIMER_INTERVAL_MSEC 250 // 250 milliseconds
#define RPM_STACK_SIZE 1024
#define RPM_THREAD_PRIORITY 7
#define RPM_COUNTER_TIME_BETWEEN_NUM_TICKS 5
