#pragma once

#include <sys/_stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "Enum.hpp"
#include "zephyr/drivers/pwm.h"
#include "zephyr/kernel/thread_stack.h"

class SpindleSpeed
{
public:
	uint16_t GetCount() const;
	uint16_t GetRequestedRPM() const;
	float GetRatio() const;
	SpindleMode GetMode() const;
	uint16_t GetPWMValue() const;
	void SetMode(SpindleMode aMode);
	SpindleSpeed();

private:
	const struct device *qdecDev = nullptr;
	const struct device *eepromDev = nullptr;
	// const struct device *pwmDev = nullptr;
	struct pwm_dt_spec pwmSpec;
	int myCount = 0;
	int myCurrentRPM = 2000;
	int myPWMValue = 0;
	float myRPMMultiplier = 50.0f;
	SpindleMode myMode = SpindleMode::IDLE;
	SpindleMode lastMode = myMode;

	struct gpio_callback buttonCbData;

	int64_t buttonPressTime = 0;

	static void qdecThread(void *aSpindleSpeed, void *, void *);
	struct k_thread qdecThreadData;
	void handleRotationEvent();
	void saveCount();
	void loadCount();
	void saveRatio();
	void loadRatio();

	void setSpindlePWM(uint16_t aDutyCycle);

	k_work_delayable saveCountWork;
	k_work_delayable saveRatioWork;
	static void saveCountWorkHandler(struct k_work *work);
	static void saveRatioWorkHandler(struct k_work *work);

public:
};
