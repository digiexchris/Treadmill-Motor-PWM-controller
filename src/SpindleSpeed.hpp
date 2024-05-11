#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "Display.hpp"
#include "Enum.hpp"

class SpindleSpeed
{
public:
private:
	const struct device *qdecDev = nullptr;
	const struct device *eepromDev = nullptr;
	const struct device *buttonDev = nullptr;
	const struct device *pwmDev = nullptr;
	Display *const myDisplay = nullptr;
	int myCount = 0;
	int myCurrentRPM = 2000;
	int myPWMValue = 0;
	float myRPMMultiplier = 50.0f;
	SpindleMode myMode = SpindleMode::IDLE;
	SpindleMode lastMode = myMode;

	struct gpio_callback buttonCbData;

	int64_t buttonPressTime = 0;

	static void qdecEventHandler(const struct device *dev,
								 const struct sensor_trigger *trigger);
	static void buttonEventHandler(const struct device *dev,
								   struct gpio_callback *cb, uint32_t pins);
	void handleRotationEvent(const struct device *dev);
	void handleButtonPress();
	void handleButtonRelease();
	void saveCount();
	void loadCount();
	void saveRatio();
	void loadRatio();

	void UpdateDisplay();

	void setSpindlePWM(uint16_t aDutyCycle);

	k_work_delayable saveCountWork;
	k_work_delayable saveRatioWork;
	static void saveCountWorkHandler(struct k_work *work);
	static void saveRatioWorkHandler(struct k_work *work);

public:
	SpindleSpeed(Display *aDisplay);
	int GetCount() const;
	float GetRatio() const;
	SpindleMode GetMode() const;
};
