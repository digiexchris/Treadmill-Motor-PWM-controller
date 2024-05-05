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

class SpindleSpeed {
private:
  const struct device *qdecDev;
  const struct device *eepromDev;
  const struct device *buttonDev;
  int myCount = 0;
  float myRatio = 0.0f;
  enum class Mode {
    IDLE,
    RUNNING,
    RATIO,
  } myMode;

  struct gpio_callback buttonCbData;

  int64_t buttonPressTime = 0;
  Mode lastMode = Mode::IDLE;

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

  k_work_delayable saveCountWork;
  k_work_delayable saveRatioWork;
  static void saveCountWorkHandler(struct k_work *work);
  static void saveRatioWorkHandler(struct k_work *work);

public:
  SpindleSpeed();
  int count() const;
  int ratio() const;
};
