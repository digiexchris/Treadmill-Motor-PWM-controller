#include "Enum.hpp"
#include "LVGLDisplay.hpp"
#include "RPMCounter.hpp"
#include "SpindleSpeed.hpp"
#include <stdio.h>
#include <sys/_stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)
#define DISPLAY0_NODE DT_ALIAS(display0)
#define RPM_PULSE_COUNTER_NODE DT_ALIAS(rpm_pulse_counter)

LOG_MODULE_REGISTER(main);
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *displayDevice = DEVICE_DT_GET(DISPLAY0_NODE);
// static const struct device *rpmPulseCounter = DEVICE_DT_GET_ANY(rpm_pulse_counter);

#define RUNBUTTON0_NODE DT_ALIAS(runbutton0)
#if !DT_NODE_HAS_STATUS(RUNBUTTON0_NODE, okay)
#error "Unsupported board: runbutton0 devicetree alias is not defined"
#endif

#define STOPBUTTON0_NODE DT_ALIAS(stopbutton0)
#if !DT_NODE_HAS_STATUS(STOPBUTTON0_NODE, okay)
#error "Unsupported board: stopbutton0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec runButton = GPIO_DT_SPEC_GET_OR(RUNBUTTON0_NODE, gpios,
																 {0});
static const struct gpio_dt_spec stopButton = GPIO_DT_SPEC_GET_OR(STOPBUTTON0_NODE, gpios,
																  {0});
static struct gpio_callback startButtonCbData;
static struct gpio_callback stopButtonCbData;

LVGLDisplay *display;
RPMCounter *rpmCounter;
SpindleSpeed *spindleSpeed;

void button_pressed(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{
	if (pins & BIT(runButton.pin))
	{
		LOG_INF("Run Button pressed at %" PRIu32 "\n", k_cycle_get_32());

		spindleSpeed->SetMode(SpindleMode::RUNNING);
	}
	else if (pins & BIT(stopButton.pin))
	{
		LOG_INF("Stop Button pressed at %" PRIu32 "\n", k_cycle_get_32());

		spindleSpeed->SetMode(SpindleMode::IDLE);
	}
}

extern void setupButtons();
extern void setupButton(const gpio_dt_spec &aButton, gpio_callback *aCb);

int main(void)
{

	LOG_INF("Hello World! %s\n", CONFIG_BOARD);

	setupButtons();

	LOG_INF("Buttons setup\n");

	rpmCounter = new RPMCounter();

	rpmCounter->Init();

	LOG_INF("RPM Counter setup\n");

	spindleSpeed = new SpindleSpeed();

	LOG_INF("Spindle Speed setup\n");

	display = new LVGLDisplay(displayDevice, 0, 5000);
	display->Init();

	LOG_INF("Display setup\n");

	LOG_INF("Done Boot! %s\n", CONFIG_BOARD);
	while (1)
	{
		if (display->IsReady())
		{
			uint16_t requestedRPM = spindleSpeed->GetRequestedRPM();
			uint16_t currentPWM = spindleSpeed->GetPWMValue();
			SpindleMode currentMode = spindleSpeed->GetMode();
			display->SetCurrentSpeed(rpmCounter->GetRPM());
			display->SetPWMValue(currentPWM);
			display->SetRequestedSpeed(requestedRPM);
			display->SetMode(currentMode);
			k_msleep(display->Update());
		}
		else
		{
			k_msleep(100000);
		}

		LOG_INF("Done Updating Display");
	}

	return 0;
}

void blink(const struct gpio_dt_spec *led, uint32_t sleep_ms)
{
	int cnt = 0;
	int ret;

	if (!device_is_ready(led->port))
	{
		LOG_INF("Error: %s device is not ready\n", led->port->name);
		return;
	}

	ret = gpio_pin_configure_dt(led, GPIO_OUTPUT);
	if (ret != 0)
	{
		LOG_INF("Error %d: failed to configure pin %d (LED0)\n", ret, led->pin);
		return;
	}

	while (1)
	{
		gpio_pin_set(led->port, led->pin, cnt % 2);

		k_msleep(sleep_ms);
		cnt++;
	}
}

void blink0(void) { blink(&led0, 1000); }

K_THREAD_DEFINE(blink0_id, 1024, blink0, NULL, NULL, NULL, 7, 0, 0);
// K_THREAD_DEFINE(uart_out_id, STACKSIZE, uart_out, NULL, NULL, NULL, PRIORITY,
// 0,
//                 0);

void setupButtons()
{
	setupButton(runButton, &startButtonCbData);
	setupButton(stopButton, &stopButtonCbData);
}

void setupButton(const gpio_dt_spec &aButton, gpio_callback *aCb)
{
	int ret;

	if (!gpio_is_ready_dt(&runButton))
	{
		LOG_INF("Error: button device %s is not ready\n",
				aButton.port->name);
		k_oops();
		return;
	}

	ret = gpio_pin_configure_dt(&aButton, GPIO_INPUT);
	if (ret != 0)
	{
		LOG_INF("Error %d: failed to configure %s pin %d\n",
				ret, aButton.port->name, aButton.pin);
		k_oops();
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&aButton,
										  GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0)
	{
		LOG_INF("Error %d: failed to configure interrupt on %s pin %d\n",
				ret, aButton.port->name, aButton.pin);
		k_oops();
		return;
	}

	gpio_init_callback(aCb, button_pressed, BIT(aButton.pin));
	gpio_add_callback(aButton.port, aCb);
	LOG_INF("Set up button at %s pin %d\n", aButton.port->name, aButton.pin);
}