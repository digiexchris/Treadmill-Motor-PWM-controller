#pragma once

#include "IDisplay.hpp"
#include <cstdint>
#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

class RPMCounter
{
public:
	RPMCounter();
	void Init();
	int16_t GetRPM();

private:
	RPMCounter *thisRPMCounter;
	// const struct device *myPulseCounter;
	struct gpio_dt_spec myPulseCounterSpec;

	uint8_t myPulseCount = 0;

	uint32_t myCurrentUptime = 0;
	uint32_t myLastUptime = 0;

	gpio_callback myRPMCallbackData;

	uint16_t myRPMValue = 0;

	void ProcessPulses();
	static void RPMPulseCallback(const struct device *aDev, struct gpio_callback *aCb, uint32_t aPins);
};