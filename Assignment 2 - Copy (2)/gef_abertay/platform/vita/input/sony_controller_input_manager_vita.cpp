#include <platform/vita/input/sony_controller_input_manager_vita.h>
#include <gef.h>
#include <scetypes.h>
#include <sceerror.h>
#include <libdbg.h>
#include <iostream>
#include <math.h>

namespace gef
{
	float SonyControllerInputManagerVita::kStickDeadZone = 0.1f;

	SonyControllerInputManagerVita::SonyControllerInputManagerVita(const Platform& platform) :
		SonyControllerInputManager(platform)
	{
		int status = Init(SCE_CTRL_MODE_DIGITALANALOG);

		SCE_DBG_ASSERT(status == SCE_OK);
	}


	SonyControllerInputManagerVita::SonyControllerInputManagerVita(const Platform& platform, const SceUInt32 sampling_mode) :
		SonyControllerInputManager(platform)
	{
		int status = Init(sampling_mode);

		SCE_DBG_ASSERT(status == SCE_OK);
	}


	SonyControllerInputManagerVita::~SonyControllerInputManagerVita()
	{
	}

	int SonyControllerInputManagerVita::Init(const SceUInt32 sampling_mode)
	{
		int status = SCE_OK;


		// Set sampling mode
		status = sceCtrlSetSamplingMode(sampling_mode);
		if(status < SCE_OK)
			std::cout << "[ERR]" << __FUNCTION__ << "::" << __LINE__ << "ret=" <<  status << std::endl;

		return status;
	}

	int SonyControllerInputManagerVita::Update()
	{
		int return_code = SCE_OK;


		/*E Read the latest controller data. */
		SceCtrlData controller_data;
		return_code = sceCtrlReadBufferPositive(0, &controller_data, 1);
		if(return_code < SCE_OK)
		{
			std::cout << "[ERR]" << __FUNCTION__ << "::" << __LINE__ << "ret=" <<  return_code << std::endl;
		}
		else
		{
			UpdateController(controller_, controller_data);
		}

		return return_code;
	}

	void SonyControllerInputManagerVita::UpdateController(SonyController& controller, const SceCtrlData& controller_data)
	{
		UInt32 previous_buttons_down;

		// get the buttons status before they are updated
		previous_buttons_down = controller.buttons_down();

		controller.set_buttons_down(controller_data.buttons);
		controller.UpdateButtonStates(previous_buttons_down);

		// calculate the stick values
		// -1 to 1 x-axis left to right
		// -1 to 1 y-axis up to down
		float left_stick_x_axis = (static_cast<float>(controller_data.lx) - 127.5f) / 127.5f;
		float left_stick_y_axis = (static_cast<float>(controller_data.ly) - 127.5f) / 127.5f;
		float right_stick_x_axis = (static_cast<float>(controller_data.rx) - 127.5f) / 127.5f;
		float right_stick_y_axis = (static_cast<float>(controller_data.ry) - 127.5f) / 127.5f;

		// if any of the stick values are less than the dead zone threshold then zero them out
		if(fabsf(left_stick_x_axis) < kStickDeadZone)
			left_stick_x_axis = 0.0f;
		if(fabsf(left_stick_y_axis) < kStickDeadZone)
			left_stick_y_axis = 0.0f;
		if(fabsf(right_stick_x_axis) < kStickDeadZone)
			right_stick_x_axis = 0.0f;
		if(fabsf(right_stick_y_axis) < kStickDeadZone)
			right_stick_y_axis = 0.0f;

		controller.set_left_stick_x_axis(left_stick_x_axis);
		controller.set_left_stick_y_axis(left_stick_y_axis);
		controller.set_right_stick_x_axis(right_stick_x_axis);
		controller.set_right_stick_y_axis(right_stick_y_axis);
	}
}