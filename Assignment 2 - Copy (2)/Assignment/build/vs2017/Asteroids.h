#pragma once
#include "game_object.h"
#include "box2d/box2d.h"
#include "primitive_builder.h"
#include <graphics/renderer_3d.h>

class Asteroids: public GameObject
{
public:
	void Init(b2World* world_, gef::Mesh* texture, float xPosition);
	void Update();
	void Render(gef::Renderer3D* renderer_3d_, PrimitiveBuilder* primitive_builder_);
	float PositionY() { return asteroid_body_->GetPosition().y; };
	Asteroids();
	~Asteroids();
private:
	b2Body* asteroid_body_;
	b2BodyDef asteroid_body_def;
	//b2Body* asteroid_body_;
	gef::MeshInstance asteroid;
};

