#include "Enviroment.h"

void Enviroment::Init(PrimitiveBuilder* primitive_builder_, b2World* world_)
{
	InitLeft(primitive_builder_, world_);
	InitRight(primitive_builder_, world_);
}

void Enviroment::Render(gef::Renderer3D* renderer_3d_)
{
	// Draws the ground 
	renderer_3d_->DrawMesh(left_);
	renderer_3d_->DrawMesh(right_);
}


void Enviroment::InitLeft(PrimitiveBuilder* primitive_builder_, b2World* world_)
{
	// ground dimensions
	gef::Vector4 left_half_dimensions(1.f, 42.f, 0.5f);

	// setup the mesh for the ground
	left_.set_mesh(primitive_builder_->CreateBoxMesh(left_half_dimensions));

	// create a physics body
	b2BodyDef body_def;
	body_def.type = b2_staticBody;
	body_def.position = b2Vec2(-74.2f, 0.0f);
	body_def.userData.pointer = reinterpret_cast<uintptr_t>(this);;

	left_body_ = world_->CreateBody(&body_def);

	// create the shape
	b2PolygonShape shape;
	shape.SetAsBox(left_half_dimensions.x(), left_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;

	// create the fixture on the rigid body
	left_body_->CreateFixture(&fixture_def);

	// update visuals from simulation data
	left_.UpdateFromSimulation(left_body_);
	left_.SetType(0);
}

void Enviroment::InitRight(PrimitiveBuilder* primitive_builder_, b2World* world_)
{
	// ground dimensions
	gef::Vector4 right_half_dimensions(1.f, 42.f, 0.5f);

	// setup the mesh for the ground
	right_.set_mesh(primitive_builder_->CreateBoxMesh(right_half_dimensions));

	// create a physics body
	b2BodyDef body_def;
	body_def.type = b2_staticBody;
	body_def.position = b2Vec2(74.2f, 0.0f);
	body_def.userData.pointer = reinterpret_cast<uintptr_t>(this);

	right_body_ = world_->CreateBody(&body_def);

	// create the shape
	b2PolygonShape shape;
	shape.SetAsBox(right_half_dimensions.x(), right_half_dimensions.y());

	// create the fixture
	b2FixtureDef fixture_def;
	fixture_def.shape = &shape;

	// create the fixture on the rigid body
	right_body_->CreateFixture(&fixture_def);

	// update visuals from simulation data
	right_.UpdateFromSimulation(right_body_);
	right_.SetType(0);
}


