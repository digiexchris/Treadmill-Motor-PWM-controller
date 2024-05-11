#pragma once

#include "Display.hpp"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

class RPMCounter
{
public:
	RPMCounter(Display *aDisplay);
	~RPMCounter();

private:
	Display *myDisplay;
	RPMCounter *thisRPMCounter;
	const struct device *myCountingTimer;
	const struct device *myProcessingTimer;
	k_thread_stack_t *myRpmStack = nullptr;
	struct k_thread myRpmThread;
	struct k_sem myRpmSem;
	uint32_t myCurrentTimerCount = 0;
	uint32_t myPreviuosTimerCount = 0;
	volatile uint32_t myPulseCount = 0;
	struct gpio_callback myGpioCbData;

	static void ProcessingTimerHandler(const struct device *aDev, void *aRPMCounter);
	void ProcessPulses(uint32_t aPulseCount);
	void InitTimers();
};