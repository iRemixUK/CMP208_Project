#pragma once
#include "game_object.h"
#include "box2d/box2d.h"
#include "primitive_builder.h"
#include <graphics/renderer_3d.h>
class Enviroment: public GameObject
{
public:
	void Init(PrimitiveBuilder* primitive_builder_, b2World* world_);
	void Render(gef::Renderer3D* renderer_3d_);
	void InitLeft(PrimitiveBuilder* primitive_builder_, b2World* world_); // Inits the left geometry
	void InitRight(PrimitiveBuilder* primitive_builder_, b2World* world_); // Inits the right geometry
	

private:
	;

	GameObject left_;
	b2Body* left_body_;

	GameObject right_;
	b2Body* right_body_;

};

