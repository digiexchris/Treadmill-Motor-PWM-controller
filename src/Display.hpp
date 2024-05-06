#pragma once

#include <lvgl.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

class Display {
public:
  Display();
  void Init();
  void SetMode(const char *mode);
  void SetValue(const char *value);

private:
  const struct device *myDisplayDevice;
  lv_obj_t *myModeLabel;
  lv_obj_t *myValueLabel;
};
