#pragma once

#include "Enum.hpp"
#include <array>
#include <lvgl.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

class Display
{
public:
	struct RPMScale
	{
		lv_obj_t *myScale;
		lv_obj_t *myRequestedLabel;
		lv_obj_t *myActualLabel;
		lv_obj_t *myPWMLabel;
		int32_t myMinValue;
		int32_t myMaxValue;
	};

	struct ModeBar
	{
		lv_obj_t *myModeLabel;
		lv_obj_t *myModeContainer;
	};

	Display(const struct device *aDisplayDevice, const uint16_t myMinRPMValue, const uint16_t myMaxRPMValue);

	void Init();
	bool IsReady();
	void SetMode(SpindleMode aMode);

	void SetRequestedSpeed(int16_t setValue);
	void SetCurrentSpeed(int16_t actualValue);
	void SetPWMValue(int16_t pwmValue);
	void SetRPMScale(const uint16_t aMinValue, const uint16_t aMaxValue);
	uint32_t Update();

private:
	bool myReady = false;
	const struct device *myDisplayDevice;
	ModeBar *myModeBar;
	lv_obj_t *myValueLabel;

	uint16_t myMinRPMValue;
	uint16_t myMaxRPMValue;

	std::array<lv_style_t *, 3> myModeStyles;

	//  main page
	lv_obj_t *myMainPage;

	void DrawMainPage(lv_obj_t *aPage);

	RPMScale *myRPMScale;

	// Generic widget functions
	void CreateScale(lv_obj_t *aParent, RPMScale *anOutScale);
};
