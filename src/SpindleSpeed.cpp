#include "SpindleSpeed.hpp"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(SpindleSpeed);

SpindleSpeed::SpindleSpeed()
    : qdecDev(DEVICE_DT_GET_ANY(qdec0)),
      eepromDev(DEVICE_DT_GET_ANY(nordic_nvmc)),
      buttonDev(DEVICE_DT_GET_ANY(encoder_button)), myCount(0),
      myMode(Mode::IDLE) {

  if (!device_is_ready(qdecDev)) {
    LOG_ERR("Error: QDEC device is not ready.");
    return;
  }
  if (!device_is_ready(eepromDev)) {
    LOG_ERR("Error: EEPROM device is not ready.");
    return;
  }
  if (!device_is_ready(buttonDev)) {
    LOG_ERR("Error: Button device is not ready.");
    return;
  }

  gpio_pin_configure(buttonDev, 0, GPIO_INPUT | GPIO_PULL_UP);
  gpio_pin_interrupt_configure(buttonDev, 0, GPIO_INT_EDGE_BOTH);
  gpio_init_callback(&buttonCbData, buttonEventHandler, BIT(0));
  gpio_add_callback(buttonDev, &buttonCbData);

  k_work_init_delayable(&saveCountWork, saveCountWorkHandler);
  k_work_init_delayable(&saveRatioWork, saveRatioWorkHandler);

  loadCount(); // Load the saved count from EEPROM at start
  loadRatio(); // Load the saved ratio from EEPROM at start

  if (myRatio == 0) {
    myRatio = 1.0f; // Default to 1.0 if no ratio is saved
    saveRatio();
  }

  struct sensor_trigger trig;
  trig.type = SENSOR_TRIG_DATA_READY;
  trig.chan = SENSOR_CHAN_ROTATION;

  if (sensor_trigger_set(qdecDev, &trig, qdecEventHandler) != 0) {
    LOG_ERR("Error setting trigger for QDEC.");
  }
}

int SpindleSpeed::count() const { return myCount; }

int SpindleSpeed::ratio() const { return myRatio; }

void SpindleSpeed::qdecEventHandler(const struct device *dev,
                                    const struct sensor_trigger *trigger) {
  SpindleSpeed *instance = CONTAINER_OF(dev, SpindleSpeed, qdecDev);
  if (instance) {
    instance->handleRotationEvent(dev);
  }
}

void SpindleSpeed::buttonEventHandler(const struct device *dev,
                                      struct gpio_callback *cb, uint32_t pins) {

  SpindleSpeed *instance = CONTAINER_OF(dev, SpindleSpeed, buttonDev);
  if (pins & BIT(0)) {
    instance->handleButtonPress();
  } else {
    instance->handleButtonRelease();
  }
}

void SpindleSpeed::handleRotationEvent(const struct device *dev) {

  struct sensor_value val;
  if (sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val) == 0) {
    switch (myMode) {
    case Mode::IDLE:
    case Mode::RUNNING:
      myCount += val.val1;
      LOG_INF("Current rotation count: %d", myCount);
      k_work_reschedule(&saveCountWork, K_SECONDS(30));
      break;
    case Mode::RATIO:
      myRatio += (float)val.val1 / 50.0f;
      LOG_INF("Current ratio: %.2f", (double)myRatio);
      k_work_reschedule(&saveRatioWork, K_SECONDS(30));
      break;
    }
  }
}

void SpindleSpeed::handleButtonPress() { buttonPressTime = k_uptime_get(); }

void SpindleSpeed::handleButtonRelease() {
  int64_t pressDuration = k_uptime_get() - buttonPressTime;

  if (pressDuration > 2000) { // Held longer than 2 seconds
    if (myMode == Mode::RATIO) {
      myMode = lastMode; // Return to the last mode

    } else {
      lastMode = myMode; // Save current mode
      myMode = Mode::RATIO;
    }
  } else if (pressDuration >= 10 &&
             pressDuration <= 2000) { // Between 10ms and 2 seconds
    if (myMode == Mode::IDLE) {
      myMode = Mode::RUNNING;
    } else if (myMode == Mode::RUNNING) {
      myMode = Mode::IDLE;
    }
  }
  // Debounce logic or very short presses can be ignored
}

void SpindleSpeed::saveCount() {
  if (eeprom_write(eepromDev, 0, &myCount, sizeof(myCount)) != 0) {
    LOG_ERR("Failed to save rotation count to EEPROM");
  }
}

void SpindleSpeed::loadCount() {
  if (eeprom_read(eepromDev, 0, &myCount, sizeof(myCount)) != 0) {
    LOG_ERR("Failed to load rotation count from EEPROM");
    myCount = 0; // Default to 0 if read fails
  }
}

void SpindleSpeed::saveRatio() {
  if (eeprom_write(eepromDev, sizeof(myCount), &myRatio, sizeof(myRatio)) !=
      0) {
    LOG_ERR("Failed to save ratio to EEPROM");
  }
}

void SpindleSpeed::loadRatio() {
  if (eeprom_read(eepromDev, sizeof(myCount), &myRatio, sizeof(myRatio)) != 0) {
    LOG_ERR("Failed to load ratio from EEPROM");
    myRatio = 0.0f; // Default to 0 if read fails
  }
}

void SpindleSpeed::saveCountWorkHandler(struct k_work *work) {
  SpindleSpeed *cont = CONTAINER_OF(work, SpindleSpeed, myCount);
  cont->saveCount();
}

void SpindleSpeed::saveRatioWorkHandler(struct k_work *work) {
  SpindleSpeed *cont = CONTAINER_OF(work, SpindleSpeed, myCount);
  cont->saveRatio();
}
