#ifndef _PLAYER_H
#define _PLAYER_H

#include "game_object.h"
#include "box2d/box2d.h"
#include <input/input_manager.h>
#include <input/sony_controller_input_manager.h>
#include <maths/math_utils.h>
#include "primitive_builder.h"
class player : public GameObject
{
public:
	void Init(b2World* world_, float xSize, float ySize);
	void Update();
	void Input(const gef::SonyController* controller);
	float Position() { return PositionX; };
	float PositionY() { return PositionY_; };
	void DecrementHealth();
	int ReturnHealth() { return health; };
	void SetHealth(int health_) { health = health_; };
private:
	b2Body* player_body_;
	b2BodyDef player_body_def;
	gef::Vector4 left_joystick_;
	gef::Vector4 right_joystick_;
	float joystick_angle;
	float JoystickX_;
	float JoystickY_;
	float PositionX;
	float PositionY_;
	int health;
};
#endif // _PLAYER_H

