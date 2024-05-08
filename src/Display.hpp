#pragma once

#include <lvgl.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

class Display {
public:
  Display(const struct device *aDisplayDevice);
  void Init();
  bool IsReady();
  void SetMode(const char *mode);
  void SetValue(const char *value);
  uint32_t Update();
  void SetSetRpmValue(int32_t aMinValue, int32_t aMaxValue, int32_t aValue);

private:
  bool myReady = false;
  const struct device *myDisplayDevice;
  lv_obj_t *myModeLabel;
  lv_obj_t *myValueLabel;

  //  main page
  lv_obj_t *myMainPage;

  void DrawMainPage(lv_obj_t *aPage);
  // lv_obj_t *myMainPageTable;
  // static void DrawTableEventCallback(lv_event_t *e);

  lv_obj_t *myMainPageSetRPMScale;

  // Generic widget functions
  void SetScaleValue(lv_obj_t *aScale, int32_t aFromMinValue,
                     int32_t aFromMaxValue, int32_t aScaledMinValue,
                     int32_t aScaledMaxValue, int32_t v);
  void CreateScale(lv_obj_t *aParent, lv_obj_t *anOutScale, const int aMinValue,
                   const int aMaxValue);

  static void ScaleValueChangedEventCallback(lv_event_t *e);
};
