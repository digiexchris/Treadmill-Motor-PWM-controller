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
  static SpindleSpeed *instance;
  const struct device *qdecDev;
  const struct device *eepromDev;
  int myCount = 0;
  float myRatio = 0.0f;

  enum class Mode {
    IDLE,
    RUNNING,
    RATIO,
  } myMode;

  static void qdec_event_handler(const struct device *aDev,
                                 const struct sensor_trigger *aTrigger) {
    if (instance) {
      instance->HandleRotationEvent(aDev);
    }
  }

  void HandleRotationEvent(const struct device *aDev) {
    struct sensor_value val;
    if (sensor_channel_get(aDev, SENSOR_CHAN_ROTATION, &val) == 0) {
      myCount += val.val1; // Increment or decrement based on the rotation

      LOG_MODULE_DECLARE(SpindleSpeed);
      LOG_INF("Current rotation count: %d", myCount);
    }
  }

  void HandleButtonEvent(const struct device *aDev) {
    // struct sensor_value val;
    // if (sensor_channel_get(aDev, SENSOR_CHAN_ROTATION, &val) == 0) {
    //     myCount += val.val1;  // Increment or decrement based on the
    //     rotation LOG_INF("Current rotation count: %d", myCount);
    // }
  }

  void SaveCount() {
    if (eeprom_write(eepromDev, 0, &myCount, sizeof(myCount)) != 0) {
      LOG_MODULE_DECLARE(SpindleSpeed);
      LOG_ERR("Failed to save rotation count to EEPROM");
    }
  }

  void LoadCount() {
    if (eeprom_read(eepromDev, 0, &myCount, sizeof(myCount)) != 0) {
      LOG_MODULE_DECLARE(SpindleSpeed);
      LOG_ERR("Failed to load rotation count from EEPROM");
      myCount = 0; // Default to 0 if read fails
    }
  }

public:
  SpindleSpeed()
      : qdecDev(DEVICE_DT_GET_ANY(nordic_qdec)),
        eepromDev(DEVICE_DT_GET_ANY(nordic_nvmc)), myCount(0) {

    instance = this;

    if (!device_is_ready(qdecDev)) {
      LOG_MODULE_DECLARE(SpindleSpeed);
      LOG_ERR("Error: QDEC device is not ready.");
      return;
    }
    if (!device_is_ready(eepromDev)) {
      LOG_MODULE_DECLARE(SpindleSpeed);
      LOG_ERR("Error: EEPROM device is not ready.");
      return;
    }

    LoadCount(); // Load the saved count from EEPROM at start

    struct sensor_trigger trig;
    trig.type = SENSOR_TRIG_DATA_READY;
    trig.chan = SENSOR_CHAN_ROTATION;

    if (sensor_trigger_set(qdecDev, &trig, qdec_event_handler) != 0) {
      LOG_MODULE_DECLARE(SpindleSpeed);
      LOG_ERR("Error setting trigger for QDEC.");
    }
  }

  ~SpindleSpeed() {
    SaveCount(); // Save the count to EEPROM before shutting down
  }

  int Count() const { return myCount; }
};
