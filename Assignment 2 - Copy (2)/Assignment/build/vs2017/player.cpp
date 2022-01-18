#include "player.h"

void player::Init(b2World* world_, float xSize, float ySize)
{
	// Create a physics body for the player
	player_body_def.type = b2_dynamicBody;
	player_body_def.position = b2Vec2(0.0f, -25.0f);
	player_body_def.userData.pointer = reinterpret_cast<uintptr_t>(this);

	player_body_ = world_->CreateBody(&player_body_def);

	// Create the shape for the player
	b2PolygonShape player_shape;
	player_shape.SetAsBox(5, 10);

	// Create the fixture
	b2FixtureDef player_fixture_def;
	player_fixture_def.shape = &player_shape;
	player_fixture_def.density = 1.0;

	// Create the fixture on the rigid body
	player_body_->CreateFixture(&player_fixture_def);

	// Initialise the left and right joystick
	left_joystick_ = gef::Vector4::kZero;
	right_joystick_ = gef::Vector4::kZero;

	// Initialise the player's health
	health = 250;
}

void player::Update()
{
	// Update visuals from simulation data
	UpdateFromSimulation(player_body_);

	gef::Matrix44 player_transform;
	player_transform.SetIdentity();
	player_transform.RotationZ(3.14); // Rotate the player 180 degress so it is facing the correct way up
	//player_transform.Scale(gef::Vector4(0.25, 0.25, 0.25));
	player_transform.SetTranslation(gef::Vector4(player_body_->GetPosition().x,
		player_body_->GetPosition().y,
		0.0f));
	set_transform(player_transform);

	PositionY_ = player_body_->GetPosition().y;
	PositionX = player_body_->GetPosition().x;

	if (player_body_->GetPosition().y != -25.00f)
	{
		player_body_->SetTransform(b2Vec2(player_body_->GetPosition().x, -25.f), 0);
		player_body_def.position = b2Vec2(PositionX, -25.f);
	}
}

void player::Input(const gef::SonyController* controller)
{
	if (controller)
	{
		// Moves the body upwards
		if (controller->buttons_down() & gef_SONY_CTRL_UP)
		{
			
			//player_body_->ApplyForceToCenter(b2Vec2(0.f, 1500.f), true);
		}

		if (controller->buttons_down() & gef_SONY_CTRL_DOWN)
		{
			
			//player_body_->ApplyForceToCenter(b2Vec2(0.f, -1500.f), true);
		}

		// Moves the body to right
		if (controller->buttons_down() & gef_SONY_CTRL_RIGHT)
		{
			
			//player_body_->ApplyForceToCenter(b2Vec2(1000.f, 0.f), true);
		}

		// Moves the body to the left
		if (controller->buttons_down() & gef_SONY_CTRL_LEFT)
		{
			
			//player_body_->ApplyForceToCenter(b2Vec2(-1000.f, 0.f), true);
		}

		// Get the angle of the left joystick so it can be used to move the player, multiplied by 1500 as this is the speed that feels good for the game
		left_joystick_.set_x(controller->left_stick_x_axis());
		//left_joystick_.set_y(controller->left_stick_y_axis());
		player_body_->ApplyForceToCenter(b2Vec2(left_joystick_.x() * 5000.f, 0), true); 

	}
}

void player::DecrementHealth()
{
	
	health -= 1;
	
}
