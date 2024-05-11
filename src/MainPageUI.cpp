#include "Display.hpp"
#include "Enum.hpp"

#include <lvgl.h>

void Display::DrawMainPage(lv_obj_t *aPage)
{
	CreateScale(aPage, myRPMScale);

	myModeBar = new ModeBar();
	myModeBar->myModeContainer = lv_obj_create(aPage);
	myModeBar->myModeLabel = lv_label_create(myModeBar->myModeContainer);

	lv_style_t *labelStyle = new lv_style_t();
	lv_style_init(labelStyle);
	lv_style_set_text_color(labelStyle, lv_color_black());
	lv_style_set_text_font(labelStyle, &lv_font_montserrat_30);

	for (lv_style_t *style : myModeStyles)
	{
		lv_style_init(style);
		lv_style_set_width(style, 210);
		lv_style_set_height(style, 100);
		lv_style_set_line_rounded(style, true);
	}

	lv_style_set_bg_color(myModeStyles[(int)SpindleMode::IDLE], lv_palette_main(LV_PALETTE_RED));
	lv_style_set_bg_color(myModeStyles[(int)SpindleMode::RUNNING], lv_palette_main(LV_PALETTE_GREEN));
	lv_style_set_bg_color(myModeStyles[(int)SpindleMode::CAL], lv_palette_main(LV_PALETTE_BLUE));

	lv_label_set_text(myModeBar->myModeLabel, "Idle");
	lv_obj_add_style(myModeBar->myModeContainer, myModeStyles[(int)SpindleMode::IDLE], 0);
	lv_obj_add_style(myModeBar->myModeLabel, labelStyle, 0);
	lv_obj_align(myModeBar->myModeContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_align(myModeBar->myModeLabel, LV_ALIGN_CENTER, 0, 0);

	Update();

	// SetRPMScaleValue(myRPMScale, 0, 100, 54, 2543, 34);
}

/**
 * A simple round scale
 */
void Display::CreateScale(lv_obj_t *aParent, RPMScale *anOutScale)
{
	/*Create an Arc*/
	anOutScale->myScale = lv_arc_create(aParent);
	anOutScale->myMinValue = myMinRPMValue;
	anOutScale->myMaxValue = myMaxRPMValue;

	lv_style_t *style = new lv_style_t();

	lv_style_t *arcStyle = new lv_style_t();
	lv_style_init(arcStyle);
	lv_style_set_arc_color(arcStyle, lv_palette_main(LV_PALETTE_BLUE));

	lv_style_init(style);
	lv_style_set_text_font(style, &lv_font_montserrat_30);

	anOutScale->myRequestedLabel = lv_label_create(anOutScale->myScale);
	lv_label_set_text(anOutScale->myRequestedLabel, "0");

	anOutScale->myActualLabel = lv_label_create(anOutScale->myScale);
	lv_label_set_text(anOutScale->myActualLabel, "0");

	anOutScale->myPWMLabel = lv_label_create(anOutScale->myScale);
	lv_label_set_text(anOutScale->myPWMLabel, "0%");

	lv_obj_add_style(anOutScale->myRequestedLabel, style, 0);
	lv_obj_add_style(anOutScale->myActualLabel, style, 0);
	lv_obj_add_style(anOutScale->myPWMLabel, style, 0);

	lv_obj_set_size(anOutScale->myScale, 200, 200);
	lv_arc_set_rotation(anOutScale->myScale, 135);
	lv_arc_set_bg_angles(anOutScale->myScale, 0, 270);
	lv_arc_set_value(anOutScale->myScale, myMinValue);
	lv_obj_center(anOutScale->myScale);
	lv_obj_align(anOutScale->myRequestedLabel, LV_ALIGN_CENTER, 0, -20);
	lv_obj_align_to(anOutScale->myActualLabel, anOutScale->myRequestedLabel, LV_ALIGN_BOTTOM_MID, 0, -20);
	lv_obj_align(anOutScale->myPWMLabel, LV_ALIGN_BOTTOM_MID, 0, -20);
	lv_obj_center(anOutScale->myActualLabel);
	lv_obj_align(anOutScale->myScale, LV_ALIGN_TOP_MID, 0, 0);

	// lv_obj_add_event_cb(anOutScale->myScale, ScaleValueChangedEventCallback,
	// 					LV_EVENT_VALUE_CHANGED, anOutScale);

	// /*Manually update the label for the first time*/
	// lv_event_send(anOutScale->myScale, LV_EVENT_VALUE_CHANGED, anOutScale);
}