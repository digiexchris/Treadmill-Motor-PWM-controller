#include "Display.hpp"
#include "Enum.hpp"

#include "Helpers.hpp"
#include <lvgl.h>
#include <widgets/lv_arc.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(displayClass, LOG_LEVEL_INF);

Display::Display(const struct device *aDisplayDevice, const uint16_t aMinRPMValue, const uint16_t aMaxRPMValue)
	: myDisplayDevice(aDisplayDevice), myModeBar(nullptr),
	  myValueLabel(nullptr), myMinRPMValue(aMinRPMValue), myMaxRPMValue(aMaxRPMValue) {}

void Display::Init()
{
	if (!myDisplayDevice)
	{
		LOG_ERR("Failed to get binding to display");
		return;
	}

	struct display_capabilities capabilities;

	myModeStyles = {new lv_style_t(), new lv_style_t(), new lv_style_t()};

	display_get_capabilities(myDisplayDevice, &capabilities);
	myMainPage = lv_obj_create(lv_scr_act());
	lv_obj_set_size(myMainPage, 240, 320);
	myRPMScale = new RPMScale();
	myRPMScale->myMinValue = myMinRPMValue;
	myRPMScale->myMaxValue = myMaxRPMValue;
	DrawMainPage(myMainPage);

	lv_task_handler();
	if (!display_blanking_off(myDisplayDevice))
	{
		LOG_ERR("Failed to turn the display blanking off");
		return;
	}

	myReady = true;
}

uint32_t Display::Update() { return lv_task_handler(); }

void Display::SetMode(SpindleMode aMode)
{
	if (aMode == SpindleMode::IDLE)
	{
		lv_label_set_text(myModeBar->myModeLabel, "IDLE");
		lv_obj_remove_style_all(myModeBar->myModeContainer);
		lv_obj_add_style(myModeBar->myModeContainer, myModeStyles[(int)SpindleMode::IDLE], 0);
	}
	else if (aMode == SpindleMode::RUNNING)
	{
		lv_label_set_text(myModeBar->myModeLabel, "RUNNING");
		lv_obj_remove_style_all(myModeBar->myModeContainer);
		lv_obj_add_style(myModeBar->myModeContainer, myModeStyles[(int)SpindleMode::RUNNING], 0);
	}
	else if (aMode == SpindleMode::CAL)
	{
		lv_label_set_text(myModeBar->myModeLabel, "CAL");
		lv_obj_remove_style_all(myModeBar->myModeContainer);
		lv_obj_add_style(myModeBar->myModeContainer, myModeStyles[(int)SpindleMode::CAL], 0);
	}
}

void Display::SetRequestedSpeed(int16_t setValue)
{
	lv_label_set_text_fmt(myRPMScale->myRequestedLabel, "%d", setValue);
}

void Display::SetCurrentSpeed(int16_t actualValue)
{
	lv_arc_set_value(myRPMScale->myScale, static_cast<int32_t>(ScaleValue(actualValue, myRPMScale->myMinValue, myRPMScale->myMaxValue, 0, 100)));
	lv_label_set_text_fmt(myRPMScale->myActualLabel, "%d", actualValue);
}

void Display::SetPWMValue(int16_t pwmValue)
{
	lv_label_set_text_fmt(myRPMScale->myPWMLabel, "%d%%", pwmValue);
}

void Display::SetRPMScale(const uint16_t aMinValue, const uint16_t aMaxValue)
{
	myRPMScale->myMinValue = aMinValue;
	myRPMScale->myMaxValue = aMaxValue;
}