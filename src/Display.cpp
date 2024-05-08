#include "Display.hpp"
#include <lvgl.h>
#include <widgets/lv_arc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(displayClass, LOG_LEVEL_INF);

const int32_t MIN_RPM_VALUE = 0;
const int32_t MAX_RPM_VALUE = 4000;

Display::Display(const struct device *aDisplayDevice)
    : myDisplayDevice(aDisplayDevice), myModeLabel(nullptr),
      myValueLabel(nullptr) {}

void Display::Init() {

  if (!myDisplayDevice) {
    LOG_ERR("Failed to get binding to display");
    return;
  }

  struct display_capabilities capabilities;

  // display_set_orientation(myDisplayDevice, DISPLAY_ORIENTATION_ROTATED_90);

  display_get_capabilities(myDisplayDevice, &capabilities);

  myMainPage = lv_obj_create(lv_scr_act());
  lv_obj_set_size(myMainPage, 240, 320);
  // lv_obj_set_style_bg_color(myMainPage, lv_palette_main(LV_PALETTE_GREY), 0);

  DrawMainPage(myMainPage);

  lv_task_handler();

  if (!display_blanking_off(myDisplayDevice)) {
    LOG_ERR("Failed to turn the display blanking off");
    return;
  }

  myReady = true;
}

uint32_t Display::Update() { return lv_task_handler(); }

/**
 * A simple round scale
 */
void Display::CreateScale(lv_obj_t *aParent, lv_obj_t *anOutScale,
                          const int32_t aMinValue, const int32_t aMaxValue) {
  TODO create a struct that can store the scale so that the min /
      max can be stored on it and updated when the calibration
          changes.lv_obj_t *label = lv_label_create(aParent);

  /*Create an Arc*/
  anOutScale = lv_arc_create(aParent);
  lv_obj_set_size(anOutScale, 150, 150);
  lv_arc_set_rotation(anOutScale, 135);
  lv_arc_set_bg_angles(anOutScale, 0, 270);
  lv_arc_set_value(anOutScale, aMinValue);
  lv_obj_center(anOutScale);
  lv_obj_add_event_cb(anOutScale, ScaleValueChangedEventCallback,
                      LV_EVENT_VALUE_CHANGED, label);

  /*Manually update the label for the first time*/
  lv_event_send(anOutScale, LV_EVENT_VALUE_CHANGED, label);
}

/*
        @brief Set the RPM value on the scale
        @param aMinValue The minimum value of the input
        @param aMaxValue The maximum value of the input
        @param aValue The value of the input between the min and max

*/
void Display::SetSetRpmValue(int32_t aMinValue, int32_t aMaxValue,
                             int32_t aValue) {
  SetScaleValue(myMainPageSetRPMScale, aMinValue, aMaxValue, MIN_RPM_VALUE,
                MAX_RPM_VALUE, aValue);
}

/**
 * @brief Set the value of the scale
 * @param aScale The scale to set the value of
 * @param aFromMinValue The minimum value of the input
 * @param aFromMaxValue The maximum value of the input
 * @param aScaledMinValue The minimum value of the scaled output
 * @param aScaledMaxValue The maximum value of the scaled output
 * @param v The value to set the scale to
 */
void Display::SetScaleValue(lv_obj_t *aScale, int32_t aFromMinValue,
                            int32_t aFromMaxValue, int32_t aScaledMinValue,
                            int32_t aScaledMaxValue, int32_t v) {
  lv_arc_set_value(aScale, v);
}

void Display::ScaleValueChangedEventCallback(lv_event_t *e) {
  lv_obj_t *arc = lv_event_get_target(e);
  lv_obj_t *label = static_cast<lv_obj_t *>(lv_event_get_user_data(e));

  lv_label_set_text_fmt(label, "%d%%", lv_arc_get_value(arc));

  /*Rotate the label to the current position of the arc*/
  lv_arc_rotate_obj_to_angle(arc, label, 25);
}