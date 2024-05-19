#if CONFIG_LVGL

#include "LVGLDisplay.hpp"
#include "Enum.hpp"

#include "Helpers.hpp"

#include "lv_api_map.h"
#include <lvgl.h>
#include <widgets/lv_arc.h>

#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(displayClass, LOG_LEVEL_INF);

LVGLDisplay::LVGLDisplay(const struct device *aDisplayDevice, const uint16_t aMinRPMValue, const uint16_t aMaxRPMValue)
	: myDisplayDevice(aDisplayDevice),
#if CONFIG_LVGL
	  myMinRPMValue(aMinRPMValue),
	  myMaxRPMValue(aMaxRPMValue),
#endif
	  myModeBar(nullptr), myValueLabel(nullptr)
{
}

void LVGLDisplay::Init()
{
	if (!myDisplayDevice)
	{
		LOG_ERR("Failed to get binding to display");
		k_oops();
		return;
	}

#if CONFIG_LVGL
	myMainPage = lv_obj_create(lv_scr_act());
	lv_obj_set_size(myMainPage, 240, 320);
	myRPMScale = new RPMScale();
	myRPMScale->myMinValue = myMinRPMValue;
	myRPMScale->myMaxValue = myMaxRPMValue;
	DrawMainPage(myMainPage);

	lv_task_handler();
	if (display_blanking_off(myDisplayDevice) != 0)
	{
		LOG_ERR("Failed to turn the display blanking off");
		k_oops();
		return;
	}

#endif

	myReady = true;
}

uint32_t LVGLDisplay::Update()
{

#if CONFIG_LVGL
	return lv_timer_handler_run_in_period(150);
#else
	return 1000;
#endif
}

void LVGLDisplay::SetMode(SpindleMode aMode)
{
#if CONFIG_LVGL
	if (aMode == SpindleMode::IDLE)
	{
		lv_label_set_text(myModeBar->myModeLabel, "IDLE");
		// lv_obj_remove_style_all(myModeBar->myModeContainer);
		lv_obj_set_style_bg_color(myModeBar->myModeContainer, lv_palette_main(LV_PALETTE_RED), 0);
		// lv_obj_add_style(myModeBar->myModeContainer, myModeStyles[(int)SpindleMode::IDLE].myStyle, 0);
	}
	else if (aMode == SpindleMode::RUNNING)
	{
		lv_label_set_text(myModeBar->myModeLabel, "RUNNING");
		// lv_obj_remove_style_all(myModeBar->myModeContainer);
		// lv_obj_add_style(myModeBar->myModeContainer, myModeStyles[(int)SpindleMode::RUNNING].myStyle, 0);
		lv_obj_set_style_bg_color(myModeBar->myModeContainer, lv_palette_main(LV_PALETTE_GREEN), 0);
	}
	else if (aMode == SpindleMode::CAL)
	{
		lv_label_set_text(myModeBar->myModeLabel, "CAL");
		lv_obj_set_style_bg_color(myModeBar->myModeContainer, lv_palette_main(LV_PALETTE_BLUE), 0);
	}
#endif
}

bool LVGLDisplay::IsReady()
{
	return myReady;
}

void LVGLDisplay::SetRequestedSpeed(int16_t setValue)
{
#if CONFIG_LVGL
	lv_label_set_text_fmt(myRPMScale->myRequestedLabel, "%d", setValue);
#endif
}

void LVGLDisplay::SetCurrentSpeed(int16_t actualValue)
{
#if CONFIG_LVGL
	lv_arc_set_value(myRPMScale->myScale, static_cast<int16_t>(ScaleValue(actualValue, myRPMScale->myMinValue, myRPMScale->myMaxValue, 0, 100)));
	lv_label_set_text_fmt(myRPMScale->myActualLabel, "%d", actualValue);
#endif
}

void LVGLDisplay::SetPWMValue(int16_t pwmValue)
{
#if CONFIG_LVGL
	lv_label_set_text_fmt(myRPMScale->myPWMLabel, "%d%%", pwmValue);
#endif
}

void LVGLDisplay::SetRPMScale(const uint16_t aMinValue, const uint16_t aMaxValue)
{
#if CONFIG_LVGL
	myRPMScale->myMinValue = aMinValue;
	myRPMScale->myMaxValue = aMaxValue;
#endif
}

#endif