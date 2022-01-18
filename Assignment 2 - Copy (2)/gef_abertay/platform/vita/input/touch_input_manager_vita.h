#ifndef _ABFW_TOUCH_INPUT_MANAGER_VITA_H
#define _ABFW_TOUCH_INPUT_MANAGER_VITA_H

#include <input/touch_input_manager.h>
#include <touch.h>
#include <gef.h>

namespace gef
{



class TouchInputManagerVita : public TouchInputManager
{
public:
	TouchInputManagerVita();
	~TouchInputManagerVita();

	void Update();

	void EnablePanel(const Int32 panel_index);
	void DisablePanel(const Int32 panel_index);

	const Int32 max_num_touches() const { return kMaxNumTouches; }
	const Int32 max_num_panels() const { return kMaxNumPanels; }
	const bool panel_enabled(const Int32 panel_index) const { return panel_enabled_[panel_index]; }

	const gef::Vector2 mouse_position() const
	{
		return Vector2(0.0f, 0.0f);
	}

	const gef::Vector4 mouse_rel() const
	{
		return Vector4(0.0f, 0.0f, 0.0f);
	}

	bool is_button_down(Int32 button_num) const
	{
		return false;
	}
private:
	Vector2 GetCursorPosition(const Int32 panel_index, const SceTouchReport& report);

	SceTouchPanelInfo panel_info_[SCE_TOUCH_PORT_MAX_NUM];
	bool panel_enabled_[SCE_TOUCH_PORT_MAX_NUM];

	static const Int32 kMaxNumTouches;
	static const Int32 kMaxNumPanels;
};

}

#endif // _ABFW_TOUCH_INPUT_MANAGER_VITA_H

