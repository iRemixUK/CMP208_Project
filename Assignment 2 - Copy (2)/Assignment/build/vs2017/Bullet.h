#pragma once
#include "game_object.h"
#include "box2d/box2d.h"
#include "primitive_builder.h"
#include <graphics/renderer_3d.h>
class Bullet: public GameObject
{
public:
	void Init(b2World* world_, float xPosition, PrimitiveBuilder* primitive_builder_);
	void Update();
	void Render(gef::Renderer3D* renderer_3d, PrimitiveBuilder* primitive_builder_);
	float PositionY() { return bullet_body_->GetPosition().y; };
	Bullet();
	~Bullet();

private:
	gef::MeshInstance bullet;
	b2Body* bullet_body_;
	b2BodyDef bullet_body_def_;
};

