#include "RPMCounter.hpp"
#include "config.hpp"
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(RPMCounter, LOG_LEVEL_INF);

RPMCounter::RPMCounter(Display *aDisplay) : myDisplay(aDisplay)
{
	if (!myDisplay->IsReady())
	{
		LOG_ERR("Error: Display::Init() has not been called yet.");
		return;
	}

	thisRPMCounter = this;
	InitTimers();
}

void RPMCounter::InitTimers()
{
	// Binding to the pulse counting timer
	myCountingTimer = device_get_binding("RPM_PULSE_COUNTER");
	if (!myCountingTimer)
	{
		LOG_ERR("Failed to get binding for counting timer");
		return;
	}

	// Binding to the processing timer
	myProcessingTimer = device_get_binding("RPM_PROCESSING_TIMER");
	if (!myProcessingTimer)
	{
		LOG_ERR("Failed to get binding for processing timer");
		return;
	}

	// Start the processing timer and configure the overflow callback
	struct counter_top_cfg myTopCfg;
	myTopCfg.ticks = counter_us_to_ticks(myProcessingTimer, 250000); // Convert 250 ms to timer ticks
	myTopCfg.callback = ProcessingTimerHandler;						 // Callback to handle the overflow event
	myTopCfg.user_data = this;										 // Passing this instance for use within the callback if needed

	if (counter_set_top_value(myProcessingTimer, &myTopCfg) < 0)
	{
		LOG_ERR("Failed to set top value for processing timer");
		return;
	}

	if (counter_start(myProcessingTimer) != 0)
	{
		LOG_ERR("Failed to start processing timer");
		return;
	}

	if (counter_start(myCountingTimer) != 0)
	{
		LOG_ERR("Failed to start counting timer");
		return;
	}
}

void RPMCounter::ProcessingTimerHandler(const struct device *aDev, void *aRPMCounter)
{
	RPMCounter *myInstance = static_cast<RPMCounter *>(aRPMCounter);
	myInstance->myPreviuosTimerCount = myInstance->myCurrentTimerCount;
	counter_get_value(myInstance->myCountingTimer, &myInstance->myCurrentTimerCount);
	uint32_t pulseCount = myInstance->myCurrentTimerCount - myInstance->myPreviuosTimerCount;
	myInstance->ProcessPulses(pulseCount);
}

void RPMCounter::ProcessPulses(uint32_t aPulseCount)
{
	LOG_INF("Processing %u pulses", aPulseCount);

	// todo this math is wrong, it should be 60 seconds divided by the time between pulses
	// otherwise as the rpm is at lower levels it will be rounding to the nearest count if
	// the count and the next count don't land on even 1/4 second intervals.
	int16_t rpm = (aPulseCount * 240);

	myDisplay->SetCurrentSpeed(rpm);
}

RPMCounter::~RPMCounter()
{
	k_free(myRpmStack);
}