#ifndef _ABFW_CONTROLLER_MANAGER_VITA_H
#define _ABFW_CONTROLLER_MANAGER_VITA_H

#include <input/sony_controller_input_manager.h>
#include <scetypes.h>
#include <ctrl.h>

namespace gef
{

class SonyControllerInputManagerVita : public SonyControllerInputManager
{
public:

	/// SonyControllerInputManagerVita class constructor
	SonyControllerInputManagerVita(const Platform& platform);

	/// SonyControllerInputManagerVita class constructor
	///
	/// @param[in] sampling_mode the sampling mode to initialise the the controllers with
	SonyControllerInputManagerVita(const Platform& platform, const SceUInt32 sampling_mode);


	/// SonyControllerInputManagerVita class destructor
	~SonyControllerInputManagerVita();

	/// Update the controller data with the current status of the buttons and analog sticks
	///
	/// @return error status from reading the controllers [0 = success]
	int Update();
/*
	/// @return the buttons that are currently held down
	inline SceUInt32 buttons_down() const { return controller_data_.buttons; }
	/// @return the buttons that have been pressed this update
	inline SceUInt32 buttons_pressed() const { return buttons_pressed_; }
	/// @return the buttons that have been released this update
	inline SceUInt32 buttons_released() const { return buttons_released_; }

	/// @return the horizontal position of the left analog stick [ -1 <-- left, 1 --> right]
	inline float left_stick_x_axis() const { return left_stick_x_axis_; }
	/// @return the vertical position of the left analog stick [ -1 <-- up, 1 --> down]
	inline float left_stick_y_axis() const { return left_stick_y_axis_; }
	/// @return the horizontal position of the right analog stick [ -1 <-- left, 1 --> right]
	inline float right_stick_x_axis() const { return right_stick_x_axis_; }
	/// @return the vertical position of the right analog stick [ -1 <-- up, 1 --> down]
	inline float right_stick_y_axis() const { return right_stick_y_axis_; }
*/
private:
	int Init(const SceUInt32 sampling_mode);

	void UpdateController(SonyController& controller, const SceCtrlData& controller_data);


//	SceCtrlData controller_data_;	// Current controller data
/*
	SceUInt32 buttons_pressed_;		// Information for pressed buttons
	SceUInt32 buttons_released_;	// Information for released buttons
	float left_stick_x_axis_;
	float left_stick_y_axis_;
	float right_stick_x_axis_;
	float right_stick_y_axis_;
*/
	static float kStickDeadZone;

};

}

#endif // _ABFW_CONTROLLER_INPUT_H