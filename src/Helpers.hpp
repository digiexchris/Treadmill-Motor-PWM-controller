#pragma once

#include "zephyr/sys/time_units.h"
inline double ScaleValue(double value, double minOld, double maxOld, double minNew, double maxNew)
{
	return (value - minOld) / (maxOld - minOld) * (maxNew - minNew) + minNew;
}

constexpr int32_t K_CYCLES_TO_MS(int32_t cycles)
{
	return (cycles * 1000) / sys_clock_hw_cycles_per_sec();
}