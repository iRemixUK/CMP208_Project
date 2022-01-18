#include <platform/vita/input/touch_input_manager_vita.h>
#include <platform/vita/system/platform_vita.h>
#include <gef.h>
#include <libdbg.h>
#include <iostream>

namespace gef
{

#define NUM_TOUCH_DATA_SETS		(64)
#define INVALID_TOUCH_ID		(128)

const Int32 TouchInputManagerVita::kMaxNumTouches = SCE_TOUCH_MAX_REPORT;
const Int32 TouchInputManagerVita::kMaxNumPanels = SCE_TOUCH_PORT_MAX_NUM;

TouchInputManagerVita::TouchInputManagerVita(void)
{
	for(Int32 panel_index=0;panel_index<SCE_TOUCH_PORT_MAX_NUM;++panel_index)
	{
		Int32 return_value = SCE_OK;
		panel_enabled_[panel_index] = false;

		// Store the panel info
		return_value = sceTouchGetPanelInfo(panel_index, &panel_info_[panel_index]);
		SCE_DBG_ASSERT(return_value == SCE_OK);

		std::cout << "Panel " << panel_index
			<< " Active X(" << panel_info_[panel_index].minAaX << "->" << panel_info_[panel_index].maxAaX
			<< ") Y(" << panel_info_[panel_index].minAaY << "->" << panel_info_[panel_index].maxAaY
			<< ") Disp X(" << panel_info_[panel_index].minDispX << "->" << panel_info_[panel_index].maxDispX
			<< ") Y(" << panel_info_[panel_index].minDispY << "->" << panel_info_[panel_index].maxDispY
			<<")" << std::endl;

		panels_.push_back(TouchContainer());
	}
}


TouchInputManagerVita::~TouchInputManagerVita(void)
{
}

void TouchInputManagerVita::Update()
{
	for (Int32 panel_index = 0; panel_index < SCE_TOUCH_PORT_MAX_NUM; ++panel_index)
	{
		// go through existing touches and remove any that were released last update
		for (TouchIterator touch = panels_[panel_index].begin(); touch != panels_[panel_index].end();)
		{
			if (touch->type == TT_RELEASED)
				touch = panels_[panel_index].erase(touch);
			else
				++touch;
		}

		if (panel_enabled_[panel_index])
		{
			SceTouchData touch_data[NUM_TOUCH_DATA_SETS];
			Int32 num_touch_datasets = sceTouchRead(panel_index, touch_data, NUM_TOUCH_DATA_SETS);
			// under normal circumstances we will just have one set of touch data
			// but touch data can be buffered if time between read calls is too long
			// the newest dataset is at the end of the buffer
			if (num_touch_datasets)
			{
				// get the newest data from the buffer
				SceTouchData* data_read = &touch_data[num_touch_datasets - 1];

				// go through touch data for this update and update existing touches or add a new touch
				for (Int32 touch_report_index = 0; touch_report_index < data_read->reportNum; ++touch_report_index)
				{
					bool touch_updated = false;
					SceTouchReport* touch_report = &data_read->report[touch_report_index];

					// update existing touches
					for (TouchIterator touch = panels_[panel_index].begin(); touch != panels_[panel_index].end();++touch)
					{
						if (touch->id == touch_report->id)
						{
							touch->type = TT_ACTIVE;
							touch->position = GetCursorPosition(panel_index, *touch_report);
							touch_updated = true;
						}
					}

					// if this touch report didn't update an existing touch then it must be a new one
					if (!touch_updated)
					{
						Touch new_touch;
						new_touch.type = TT_NEW;
						new_touch.position = GetCursorPosition(panel_index, *touch_report);
						new_touch.id = touch_report->id;
						AddTouch(panel_index, new_touch);
					}
				}

				// no go through existing touch data and check to so if they exist in data for this update
				// if not they have been released
				for (TouchIterator touch = panels_[panel_index].begin(); touch != panels_[panel_index].end();++touch)
				{
					bool touch_exists = false;
					for (Int32 touch_report_index = 0; touch_report_index < data_read->reportNum; ++touch_report_index)
					{
						SceTouchReport* touch_report = &data_read->report[touch_report_index];
						if (touch->id == touch_report->id)
							touch_exists = true;
					}

					if (!touch_exists)
						touch->type = TT_RELEASED;
				}
			}
		}
	}

}

void TouchInputManagerVita::EnablePanel(const Int32 panel_index)
{
		Int32 return_value = SCE_OK;
		return_value = sceTouchSetSamplingState(panel_index, SCE_TOUCH_SAMPLING_STATE_START);
		SCE_DBG_ASSERT(return_value == SCE_OK);
		panel_enabled_[panel_index] = true;
}

void TouchInputManagerVita::DisablePanel(const Int32 panel_index)
{
		Int32 return_value = SCE_OK;
		return_value = sceTouchSetSamplingState(panel_index, SCE_TOUCH_SAMPLING_STATE_STOP);
		SCE_DBG_ASSERT(return_value == SCE_OK);
		panel_enabled_[panel_index] = false;
}

Vector2 TouchInputManagerVita::GetCursorPosition(const Int32 panel_index, const SceTouchReport& report)
{
	Vector2 position;

	// Convert to screen coordinates
	float ratioX = (report.x - panel_info_[panel_index].minAaX) / (float)(panel_info_[panel_index].maxAaX  - panel_info_[panel_index].minAaX + 1);
	float ratioY = (report.y - panel_info_[panel_index].minAaY) / (float)(panel_info_[panel_index].maxAaY  - panel_info_[panel_index].minAaY + 1);
	position.x = ratioX * (float)DISPLAY_WIDTH;
	position.y = ratioY * (float)DISPLAY_HEIGHT;
//	position.x = position.x - DISPLAY_WIDTH*0.5f;
//	position.y = DISPLAY_HEIGHT *0.5f - position.y;

	return position;
}

}