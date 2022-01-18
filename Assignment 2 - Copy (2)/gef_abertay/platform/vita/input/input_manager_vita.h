#ifndef _PLATFORM_VITA_INPUT_INPUT_MANAGER_H
#define _PLATFORM_VITA_INPUT_INPUT_MANAGER_H

#include <input/input_manager.h>

namespace gef
{
	class Platform;

	class InputManagerVita : public InputManager
	{
	public:
		InputManagerVita(Platform& platform);
		~InputManagerVita();
	protected:
		Platform& platform_;
	};
}
#endif // _PLATFORM_VITA_INPUT_INPUT_MANAGER_H