#pragma once

#include "Enum.hpp"

// #include <array>
#include <cstdint>
#if CONFIG_LVGL
#include "core/lv_obj.h"
#include <lvgl.h>
#endif
#include <sys/_stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

class Display
{
public:
#if CONFIG_LVGL
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
#endif

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

	uint16_t myMinRPMValue;
	uint16_t myMaxRPMValue;
#if CONFIG_LVGL
	ModeBar *myModeBar;
	lv_obj_t *myValueLabel;
	struct PageStyle
	{
		lv_style_t *myStyle;
	};

	PageStyle myModeStyles[3];

	//  main page
	lv_obj_t *myMainPage;

	void DrawMainPage(lv_obj_t *aPage);

	RPMScale *myRPMScale;

	// Generic widget functions
	void CreateScale(lv_obj_t *aParent, RPMScale *anOutScale);
#endif
};
