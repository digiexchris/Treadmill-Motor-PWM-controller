#include "RPMCounter.hpp"
#include "config.hpp"
#include "zephyr/devicetree.h"
#include "zephyr/sys/util.h"
#include <cstdint>
#include <zephyr/device.h>
#include <zephyr/devicetree/gpio.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(RPMCounter, LOG_LEVEL_INF);

#define RPMPULSE_COUNTER_NODE DT_NODELABEL(rpmpulsecounter)

#if DT_NODE_HAS_STATUS(RPMPULSE_COUNTER_NODE, okay)
#define GPIO_DEV DT_GPIO_LABEL(RPMPULSE_COUNTER_NODE, gpios)
#define GPIO_PIN DT_GPIO_PIN(RPMPULSE_COUNTER_NODE, gpios)
#define GPIO_FLAGS DT_GPIO_FLAGS(RPMPULSE_COUNTER_NODE, gpios)
#else
#error "Unsupported board: rpmpulsecounter node not defined or not enabled"
#endif

RPMCounter::RPMCounter()
{
	myPulseCounterSpec = GPIO_DT_SPEC_GET(RPMPULSE_COUNTER_NODE, gpios);
	if (!gpio_is_ready_dt(&myPulseCounterSpec))
	{
		LOG_ERR("Device not ready\n");
		k_oops();
		return;
	}

	k_mutex_init(&myPulseCountMutex);
	thisRPMCounter = this;
	myCurrentUptime = k_uptime_get_32();
	myLastUptime = myCurrentUptime;

	gpio_pin_configure_dt(&myPulseCounterSpec, GPIO_INPUT | GPIO_PULL_DOWN);
	gpio_pin_interrupt_configure_dt(&myPulseCounterSpec, GPIO_INT_EDGE_RISING);
	gpio_init_callback(&myRPMCallbackData, RPMPulseCallback, BIT(myPulseCounterSpec.pin));
	gpio_add_callback(myPulseCounterSpec.port, &myRPMCallbackData);
}

void RPMCounter::Init()
{
}

void RPMCounter::RPMPulseCallback(const struct device *aDev, struct gpio_callback *aCb, uint32_t aPins)
{
	RPMCounter *rpmCounter = CONTAINER_OF(aCb, class RPMCounter, myRPMCallbackData);

	rpmCounter->myPulseCount++;
	if (rpmCounter->myPulseCount >= RPM_COUNTER_TIME_BETWEEN_NUM_TICKS)
	{
		rpmCounter->ProcessPulses();
		rpmCounter->myPulseCount = 0;
	}
}

int16_t RPMCounter::GetRPM()
{
	if (myCurrentUptime + 1000 < k_uptime_get_32())
	{
		myRPMValue = 0;
	}

	return myRPMValue;
}

void RPMCounter::ProcessPulses()
{
	myLastUptime = myCurrentUptime;

	myCurrentUptime = k_uptime_get_32();

	uint32_t timeDiff = myCurrentUptime - myLastUptime;

	// Calculate the time difference in seconds
	float timeInSeconds = timeDiff / 1000.0f; // Convert milliseconds to seconds

	// Calculate revolutions per second
	float revolutionsPerSecond = 1 / (timeInSeconds / RPM_COUNTER_TIME_BETWEEN_NUM_TICKS);

	// Convert revolutions per second to revolutions per minute
	float rpm = revolutionsPerSecond * 60;

	myRPMValue = static_cast<int16_t>(rpm);

	printk("RPM: %d\n", myRPMValue);

	// Display the calculated RPM on the display
	// if (myDisplay->IsReady())
	// {
	// 	myDisplay->SetCurrentSpeed(static_cast<uint16_t>(rpm));
	// }
}