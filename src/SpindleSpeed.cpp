#include "SpindleSpeed.hpp"
#include "config.hpp"

#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(SpindleSpeed);

SpindleSpeed::SpindleSpeed()
    : qdecDev(DEVICE_DT_GET_ANY(qdec0)), eepromDev(DEVICE_DT_GET_ANY(eeprom0)),
      buttonDev(DEVICE_DT_GET_ANY(encoder_button)),
      pwmDev(DEVICE_DT_GET_ANY(spindlepwm)) {

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

  if (!device_is_ready(pwmDev)) {
    LOG_ERR("Error: PWM device is not ready.");
    return;
  }

  pwm_set_cycles(pwmDev, 0, PWM_FREQUENCY, 0, PWM_POLARITY_NORMAL);

  gpio_pin_configure(buttonDev, 0, GPIO_INPUT | GPIO_PULL_UP);
  gpio_pin_interrupt_configure(buttonDev, 0, GPIO_INT_EDGE_BOTH);
  gpio_init_callback(&buttonCbData, buttonEventHandler, BIT(0));
  gpio_add_callback(buttonDev, &buttonCbData);

  k_work_init_delayable(&saveCountWork, saveCountWorkHandler);
  k_work_init_delayable(&saveRatioWork, saveRatioWorkHandler);

  loadCount(); // Load the saved count from EEPROM at start
  loadRatio(); // Load the saved ratio from EEPROM at start

  if (myRPMMultiplier == 0) {
    myRPMMultiplier = 1.0f; // Default to 1.0 if no ratio is saved
    saveRatio();
  }

  struct sensor_trigger trig;
  trig.type = SENSOR_TRIG_DATA_READY;
  trig.chan = SENSOR_CHAN_ROTATION;

  if (sensor_trigger_set(qdecDev, &trig, qdecEventHandler) != 0) {
    LOG_ERR("Error setting trigger for QDEC.");
  }
}

int SpindleSpeed::GetCount() const { return myCount; }

float SpindleSpeed::GetRatio() const { return myRPMMultiplier; }

SpindleSpeed::Mode SpindleSpeed::GetMode() const { return myMode; }

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

/*
 * @brief Set the spindle speed controller's PWM duty cycle
 * @param aDutyCycle The duty cycle percent (0 to 100)
 */
void SpindleSpeed::setSpindlePWM(uint16_t aDutyCycle) {
  uint32_t period = (aDutyCycle / 100 * PWM_FREQUENCY);
  pwm_set_cycles(pwmDev, 0, PWM_FREQUENCY, period, PWM_POLARITY_NORMAL);
}

void SpindleSpeed::handleRotationEvent(const struct device *dev) {
  struct sensor_value val;
  if (sensor_channel_get(dev, SENSOR_CHAN_ROTATION, &val) == 0) {
    switch (myMode) {
    case Mode::IDLE:
    case Mode::RUNNING:
      myCount += val.val1;
      if (myCount < 0) {
        myCount = 0;
      }

      if (myCount > 100) {
        myCount = 100;
      }

      LOG_INF("Current rotation count: %d", myCount);
      k_work_reschedule(&saveCountWork, K_SECONDS(30));

      if (myMode == Mode::RUNNING) {
        setSpindlePWM(myCount);
      } else {
        setSpindlePWM(0);
      }

      // todo queue work to update the display with the new calculated rpm based
      // on mycount

      break;
    case Mode::CAL:
      myRPMMultiplier += (float)val.val1;
      LOG_INF("Current ratio: %.2f", (double)myRPMMultiplier);
      k_work_reschedule(&saveRatioWork, K_SECONDS(30));
      // todo queue work to update the display with the new ratio and new
      // calculated rpm
      break;
    }
  }
}

void SpindleSpeed::handleButtonPress() { buttonPressTime = k_uptime_get(); }

void SpindleSpeed::handleButtonRelease() {
  int64_t pressDuration = k_uptime_get() - buttonPressTime;

  if (pressDuration > 2000) { // Held longer than 2 seconds
    if (myMode == Mode::CAL) {
      myMode = lastMode; // Return to the last mode
      // todo queue work for the display to switch back to last mode
    } else {
      lastMode = myMode; // Save current mode
      myMode = Mode::CAL;
      // todo queue work to switch the display mode to CAL+Ratio and the value
      // to the calculated RPM
    }
  } else if (pressDuration >= 10 &&
             pressDuration <= 2000) { // Between 10ms and 2 seconds
    if (myMode == Mode::IDLE) {
      myMode = Mode::RUNNING;
      setSpindlePWM(myCount);
      // todo queue work for the display to switch to running mode and set the
      // value to the mycount * multiplier
    } else if (myMode == Mode::RUNNING) {
      myMode = Mode::IDLE;
      setSpindlePWM(0);
      // todo queue work for the display to switch to idle mode and set the
      // value to the mycount * multiplier
    }
  }
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
  if (eeprom_write(eepromDev, sizeof(myCount), &myRPMMultiplier,
                   sizeof(myRPMMultiplier)) != 0) {
    LOG_ERR("Failed to save ratio to EEPROM");
  }
}

void SpindleSpeed::loadRatio() {
  if (eeprom_read(eepromDev, sizeof(myCount), &myRPMMultiplier,
                  sizeof(myRPMMultiplier)) != 0) {
    LOG_ERR("Failed to load ratio from EEPROM");
    myRPMMultiplier = 0.0f; // Default to 0 if read fails
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
