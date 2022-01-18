#include "input_manager_vita.h"
#include "touch_input_manager_vita.h"
#include "sony_controller_input_manager_vita.h"
#include <platform/vita/system/platform_vita.h>

namespace gef
{
	InputManager* InputManager::Create(Platform& platform)
	{
		return new InputManagerVita(platform);
	}

	InputManagerVita::InputManagerVita(Platform& platform)
		: InputManager(platform)
		, platform_(platform)
	{

			touch_manager_ = new TouchInputManagerVita();
			platform.set_touch_input_manager(touch_manager_);
			controller_manager_ = new SonyControllerInputManagerVita(platform);
	}

	InputManagerVita::~InputManagerVita()
	{
		delete touch_manager_;
		platform_.set_touch_input_manager(NULL);

		delete controller_manager_;
	}

}