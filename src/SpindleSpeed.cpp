#include "SpindleSpeed.hpp"
#include "Enum.hpp"
#include "Helpers.hpp"
#include "config.hpp"
#include "zephyr/devicetree/pwms.h"
#include "zephyr/drivers/sensor.h"

#include <sys/_stdint.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(SpindleSpeed);

#define QDEC_NODE DT_ALIAS(qdec0)
#define EEPROM_NODE DT_ALIAS(eeprom0)
#define PWM_NODE DT_ALIAS(spindlepwm0)

// #if DT_NODE_HAS_STATUS(PWM_NODE, okay)
// // #define PWM_CTRLR DT_PWMS_(PWM_NODE, pwms)
// #define PWM_CHANNEL DT_PWMS_CHANNEL_BY_IDX(PWM_NODE, 0)
// #define PWM_PERIOD DT_PWMS_CHANNEL_BY_IDX(PWM_NODE, 0)
// #define PWM_FLAGS DT_PWMS_CHANNEL_BY_IDX(PWM_NODE, 0)
// #else
// #error "Unsupported board: spindlepwm node not defined or not enabled"
// #endif

K_THREAD_STACK_DEFINE(qdecThreadStack, 1024);

// const struct pwm_dt_spec pwmSpec = PWM_DT_SPEC_GET(PWM_NODE);

SpindleSpeed::SpindleSpeed()
{

	pwmSpec = PWM_DT_SPEC_GET(PWM_NODE);

	qdecDev = DEVICE_DT_GET(QDEC_NODE);

	if (!device_is_ready(qdecDev))
	{
		LOG_ERR("Error: QDEC device is not ready.");
		k_oops();
		return;
	}

	uint16_t preloadVal = 0;
	//  idle for a bit to let the QDEC settle (there are a few erroneous samples depending on which pin is high or low on start)
	for (int i = 0; i != 5; i++)
	{
		struct sensor_value val;
		if (sensor_sample_fetch(qdecDev) < 0)
		{
			LOG_ERR("Failed to fetch sample");
			continue;
		}

		if (sensor_channel_get(qdecDev, SENSOR_CHAN_ROTATION, &val) < 0)
		{
			LOG_ERR("Failed to get data");
			continue;
		}

		preloadVal = val.val1;
		k_sleep(K_MSEC(50));
	}

	myPreviousEncoderVal = preloadVal;

	k_thread_create(&qdecThreadData, qdecThreadStack, K_THREAD_STACK_SIZEOF(qdecThreadStack),
					qdecThread, this, NULL, NULL, 6, 0, K_NO_WAIT);

	if (!pwm_is_ready_dt(&pwmSpec))
	{
		LOG_ERR("Error: PWM device is not ready.");
		k_oops();
		return;
	}

	int ret = pwm_set_pulse_dt(&pwmSpec, 0);
	if (ret < 0)
	{
		LOG_ERR("Error %d: failed to set pulse width\n", ret);
		k_oops();
		return;
	}
}

void SpindleSpeed::SetMode(SpindleMode aMode)
{
	if (aMode == myMode)
	{
		return;
	}

	myMode = aMode;

	switch (myMode)
	{
	case SpindleMode::IDLE:
		setSpindlePWM(0);
		break;
	case SpindleMode::RUNNING:
		setSpindlePWM(myCount);
		break;
	case SpindleMode::CAL:
		break;
	}
}

uint16_t SpindleSpeed::GetPWMValue() const { return myPWMValue; }

uint16_t SpindleSpeed::GetCount() const { return myCount; }

float SpindleSpeed::GetRatio() const { return myRPMMultiplier; }

SpindleMode SpindleSpeed::GetMode() const { return myMode; }

void SpindleSpeed::qdecThread(void *aSpindleSpeed, void *, void *)
{
	SpindleSpeed *instance = static_cast<SpindleSpeed *>(aSpindleSpeed);

	if (instance)
	{
		while (1)
		{
			instance->handleRotationEvent();
			k_sleep(K_MSEC(33));
		}
	}
	else
	{
		LOG_ERR("Failed to get instance of SpindleSpeed");
		k_oops();
	}
}

uint16_t SpindleSpeed::GetRequestedRPM() const
{
	return (myCount * myRPMMultiplier);
}

/*
 * @brief Set the spindle speed controller's PWM duty cycle
 * @param aDutyCycle The duty cycle percent (0 to 100)
 */
void SpindleSpeed::setSpindlePWM(uint16_t aDutyCycle)
{
	if (aDutyCycle == myPWMValue)
	{
		return;
	}

	uint32_t pulse = static_cast<uint32_t>(aDutyCycle / 100.0f * PWM_FREQUENCY);
	myPWMValue = aDutyCycle;

	// int ret = pwm_set_pulse_dt(&pwmSpec, pulse);
	int test = pwmSpec.period;
	int ret = pwm_set(pwmSpec.dev, pwmSpec.channel, PWM_FREQUENCY, pulse,
					  pwmSpec.flags);
	if (ret < 0)
	{
		LOG_ERR("Error %d: failed to set pulse width\n", ret);
		k_oops();
		return;
	}
	else
	{
		LOG_INF("PWM set to %d", pulse);
	}
	// pwm_set_cycles(pwmSpec.dev, pwmSpec.channel, pwmSpec.period, pulse, pwmSpec.flags);
}

void SpindleSpeed::RotationTriggerHandler(const struct device *dev, const struct sensor_trigger *trigger)
{
	SpindleSpeed *instance = CONTAINER_OF(dev, SpindleSpeed, qdecDev);

	if (instance)
	{
		instance->handleRotationEvent();
	}
	else
	{
		LOG_ERR("Failed to get instance of SpindleSpeed");
		k_oops();
	}
}

void SpindleSpeed::handleRotationEvent()
{
	struct sensor_value val;
	if (sensor_sample_fetch(qdecDev) < 0)
	{
		LOG_ERR("Failed to fetch sample");
		return;
	}

	if (sensor_channel_get(qdecDev, SENSOR_CHAN_ROTATION, &val) < 0)
	{
		LOG_ERR("Failed to get data");
		return;
	}

#if DEBUG_QDEC
	LOG_INF("Val %d", val.val1);
#endif

	myPreviousCount = myCount;

	uint32_t currentVal = val.val1;

	int32_t delta = 0;

	// Detect direction and calculate delta
	if (currentVal < myPreviousEncoderVal)
	{
		if ((myPreviousEncoderVal - currentVal) > (ENCODER_MAX_VAL / 2))
		{
			// Handle overflow case (forward rotation wrapping around)
			delta = (currentVal + (ENCODER_MAX_VAL - myPreviousEncoderVal) + 1);
		}
		else
		{
			// Normal decrement (backward rotation)
			delta = -(myPreviousEncoderVal - currentVal);
		}
	}
	else
	{
		if ((currentVal - myPreviousEncoderVal) > (ENCODER_MAX_VAL / 2))
		{
			// Handle underflow case (backward rotation wrapping around)
			delta = -((myPreviousEncoderVal + (ENCODER_MAX_VAL - currentVal) + 1));
		}
		else
		{
			// Normal increment (forward rotation)
			delta = (currentVal - myPreviousEncoderVal);
		}
	}

	myPreviousEncoderVal = currentVal;

	switch (myMode)
	{
	case SpindleMode::IDLE:
	case SpindleMode::RUNNING:

		if (delta != 0)
		{
			myCount += delta;
			if (myCount < 0)
			{
				myCount = 0;
			}

			if (myCount > 100)
			{
				myCount = 100;
			}

#if DEBUG_QDEC
			LOG_INF("Current rotation count: %d", myCount);
			LOG_INF("Count %d", myCount);
#endif
			if (myPreviousCount != myCount)
			{

				// k_work_reschedule(&saveCountWork, K_SECONDS(30));
			}
		}

		if (myMode == SpindleMode::RUNNING)
		{

			setSpindlePWM(myCount);
		}
		else
		{
			setSpindlePWM(0);
		}

		// todo queue work to update the display with the new calculated rpm based
		// on mycount

		break;
	case SpindleMode::CAL:
		float myPreviousRPMMultiplier = myRPMMultiplier;
		myRPMMultiplier += (float)val.val1;
		LOG_INF("Current ratio: %.2f", (double)myRPMMultiplier);
		if (myRPMMultiplier != myPreviousRPMMultiplier)
		{
			// k_work_reschedule(&saveRatioWork, K_SECONDS(30));
		}

		// todo queue work to update the display with the new ratio and new
		// calculated rpm
		break;
	}
}

void SpindleSpeed::saveCount()
{
	if (eeprom_write(eepromDev, 0, &myCount, sizeof(myCount)) != 0)
	{
		LOG_ERR("Failed to save rotation count to EEPROM");
	}
}

void SpindleSpeed::loadCount()
{
	if (eeprom_read(eepromDev, 0, &myCount, sizeof(myCount)) != 0)
	{
		LOG_ERR("Failed to load rotation count from EEPROM");
		myCount = 0; // Default to 0 if read fails
	}

	// TODO rescale the display based on the loaded rpm multiplier
}

void SpindleSpeed::saveRatio()
{
	if (eeprom_write(eepromDev, sizeof(myCount), &myRPMMultiplier,
					 sizeof(myRPMMultiplier)) != 0)
	{
		LOG_ERR("Failed to save ratio to EEPROM");
	}
}

void SpindleSpeed::loadRatio()
{
	if (eeprom_read(eepromDev, sizeof(myCount), &myRPMMultiplier,
					sizeof(myRPMMultiplier)) != 0)
	{
		LOG_ERR("Failed to load ratio from EEPROM");
		myRPMMultiplier = 0.0f; // Default to 0 if read fails
	}
}

void SpindleSpeed::saveCountWorkHandler(struct k_work *work)
{
	SpindleSpeed *cont = CONTAINER_OF(work, SpindleSpeed, myCount);
	cont->saveCount();
}

void SpindleSpeed::saveRatioWorkHandler(struct k_work *work)
{
	SpindleSpeed *cont = CONTAINER_OF(work, SpindleSpeed, myCount);
	cont->saveRatio();
}
