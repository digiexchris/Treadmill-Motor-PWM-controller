#include "SpindleSpeed.hpp"
#include "Enum.hpp"
#include "Helpers.hpp"
#include "config.hpp"
#include "zephyr/devicetree/pwms.h"

#include <sys/_stdint.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(SpindleSpeed);

#define QDEC_NODE DT_ALIAS(qdec0)
#define EEPROM_NODE DT_ALIAS(eeprom0)
#define PWM_NODE DT_NODELABEL(spindle1)

#if DT_NODE_HAS_STATUS(PWM_NODE, okay)
// #define PWM_CTRLR DT_PWMS_(PWM_NODE, pwms)
#define PWM_CHANNEL DT_PWMS_CHANNEL_BY_IDX(PWM_NODE, 0)
#define PWM_PERIOD DT_PWMS_CHANNEL_BY_IDX(PWM_NODE, 0)
#define PWM_FLAGS DT_PWMS_CHANNEL_BY_IDX(PWM_NODE, 0)
#else
#error "Unsupported board: spindlepwm node not defined or not enabled"
#endif

K_THREAD_STACK_DEFINE(qdecThreadStack, 1024);

// const struct pwm_dt_spec pwmSpec = PWM_DT_SPEC_GET(PWM_NODE);

SpindleSpeed::SpindleSpeed()
{

	// const struct device *pwmDevice = PWM_DT_SPEC_INST_GET(PWM_NODE);
	pwmSpec = PWM_DT_SPEC_GET(PWM_NODE);

	// DT_PWMS_CELL(PWM_NODE, cell)

	// // eepromDev = DEVICE_DT_GET(EEPROM_NODE);
	// pwmDev = DEVICE_DT_GET(PWM_NODE);

	// auto channel = DT_PWMS_CHANNEL(DT_NODELABEL(spindle));

	qdecDev = DEVICE_DT_GET(QDEC_NODE);

	if (!device_is_ready(qdecDev))
	{
		LOG_ERR("Error: QDEC device is not ready.");
		k_oops();
		return;
	}

	k_thread_create(&qdecThreadData, qdecThreadStack, K_THREAD_STACK_SIZEOF(qdecThreadStack),
					qdecThread, this, NULL, NULL, 7, 0, K_NO_WAIT);

	// if (!device_is_ready(eepromDev))
	// {
	// 	LOG_ERR("Error: EEPROM device is not ready.");
	// 	k_oops();
	// 	return;
	// }

	if (!device_is_ready(pwmSpec.dev))
	{
		LOG_ERR("Error: PWM device is not ready.");
		k_oops();
		return;
	}

	pwm_set_cycles(pwmSpec.dev, pwmSpec.channel, pwmSpec.period, 0, pwmSpec.flags);

	// k_work_init_delayable(&saveCountWork, saveCountWorkHandler);
	// k_work_init_delayable(&saveRatioWork, saveRatioWorkHandler);

	// // TODO extact settings from this class, make it it's own class. Need to pass these values into both display and this class on construction.
	// loadRatio(); // Load the saved ratio from EEPROM at start

	// if (myRPMMultiplier == 0)
	// {
	// 	myRPMMultiplier = 50.0f; // Default to 50.0 if no ratio is saved for 5000 rpm
	// 	saveRatio();
	// }
	// loadCount(); // Load the saved count from EEPROM at start
}

void SpindleSpeed::SetMode(SpindleMode aMode)
{
	if (aMode == myMode)
	{
		return;
	}

	myMode = aMode;
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
			k_sleep(K_MSEC(100));
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
	uint32_t pulse = (aDutyCycle / 100 * pwmSpec.period);
	pwm_set_cycles(pwmSpec.dev, pwmSpec.channel, pwmSpec.period, pulse, pwmSpec.flags);
}

void SpindleSpeed::handleRotationEvent()
{
	struct sensor_value val;
	if (sensor_channel_get(qdecDev, SENSOR_CHAN_ROTATION, &val) == 0)
	{
		int16_t currentCount = myCount;
		switch (myMode)
		{
		case SpindleMode::IDLE:
		case SpindleMode::RUNNING:
			myCount += val.val1;
			if (myCount < 0)
			{
				myCount = 0;
			}

			if (myCount > 100)
			{
				myCount = 100;
			}

			LOG_INF("Current rotation count: %d", myCount);
			if (currentCount != myCount)
			{
				k_work_reschedule(&saveCountWork, K_SECONDS(30));
			}

			if (myMode == SpindleMode::RUNNING)
			{
				if (myCount != currentCount)
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
				k_work_reschedule(&saveRatioWork, K_SECONDS(30));
			}

			// todo queue work to update the display with the new ratio and new
			// calculated rpm
			break;
		}
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
