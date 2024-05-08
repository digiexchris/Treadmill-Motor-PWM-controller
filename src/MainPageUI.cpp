#include "Display.hpp"

#include <lvgl.h>

void Display::DrawMainPage(lv_obj_t *aPage) {
  CreateScale(aPage, myMainPageSetRPMScale);
  Update();

  SetScaleValue(myMainPageSetRPMScale, 20);
}