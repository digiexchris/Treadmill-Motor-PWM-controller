#pragma once

#include "Enum.hpp"

#if CONFIG_LVGL
#include "IDisplay.hpp"
#include "core/lv_obj.h"
#include <cstdint>
#include <lvgl.h>
#include <sys/_stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>

class LVGLDisplay : public IDisplay
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

	LVGLDisplay(const struct device *aDisplayDevice, const uint16_t myMinRPMValue, const uint16_t myMaxRPMValue);
	virtual void Init() override;
	virtual void SetMode(SpindleMode aMode) override;
	virtual bool IsReady() override;
	virtual void SetRequestedSpeed(int16_t setValue) override;
	virtual void SetCurrentSpeed(int16_t actualValue) override;
	virtual void SetPWMValue(int16_t pwmValue) override;
	virtual void SetRPMScale(const uint16_t aMinValue, const uint16_t aMaxValue) override;
	virtual uint32_t Update() override;

private:
	bool myReady = false;
	const struct device *myDisplayDevice;

	uint16_t myMinRPMValue;
	uint16_t myMaxRPMValue;

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
};
#endif