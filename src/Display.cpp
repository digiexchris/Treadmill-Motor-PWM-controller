#include "Display.hpp"
#include <lvgl.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(displayClass, LOG_LEVEL_INF);

Display::Display()
    : myDisplayDevice(nullptr), myModeLabel(nullptr), myValueLabel(nullptr) {}

void Display::Init() {
  myDisplayDevice = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!myDisplayDevice) {
    LOG_ERR("Failed to get binding to SSD1306");
    return;
  }

  myModeLabel = lv_label_create(lv_scr_act());
  lv_obj_align(myModeLabel, LV_ALIGN_CENTER, 0, 0);
  lv_label_set_text(myModeLabel, "IDLE");

  myValueLabel = lv_label_create(lv_scr_act());
  lv_obj_align(myValueLabel, LV_ALIGN_BOTTOM_LEFT, 0, 20);
  lv_label_set_text_fmt(myValueLabel, "RPM: %d", 0);
}
