#include "Asteroids.h"

void Asteroids::Init(b2World* world_, gef::Mesh* texture, float xPosition)
{
	// Create a physics body for the player
	asteroid_body_def.type = b2_dynamicBody;
	asteroid_body_def.position = b2Vec2(xPosition, 55.0f);
	asteroid_body_def.userData.pointer = reinterpret_cast<uintptr_t>(this);

	asteroid_body_ = world_->CreateBody(&asteroid_body_def);

	// Create the shape for the player
	b2PolygonShape asteroid_shape;
	asteroid_shape.SetAsBox(10, 10);

	// Create the fixture
	b2FixtureDef asteroid_fixture_def;
	asteroid_fixture_def.shape = &asteroid_shape;
	asteroid_fixture_def.density = 0.1;

	// Create the fixture on the rigid body
	asteroid_body_->CreateFixture(&asteroid_fixture_def);

	// Set the mesh for the asteroid
	asteroid.set_mesh(texture);
}

void Asteroids::Update()
{
	// Update simulation
	UpdateFromSimulation(asteroid_body_);

	gef::Matrix44 asteroid_transform;
	asteroid_transform.SetIdentity();
	asteroid_transform.RotationZ(asteroid_body_->GetAngle());
	asteroid_transform.Scale(gef::Vector4(0.17, 0.17, 0.17));
	asteroid_transform.SetTranslation(gef::Vector4(asteroid_body_->GetPosition().x,
		asteroid_body_->GetPosition().y,
		0.0f));
	asteroid.set_transform(asteroid_transform);

	asteroid_body_->ApplyForceToCenter(b2Vec2(0.0f, -25.f), true);
}

void Asteroids::Render(gef::Renderer3D* renderer_3d_, PrimitiveBuilder* primitive_builder_)
{
	renderer_3d_->DrawMesh(asteroid);
	renderer_3d_->set_override_material(NULL);
}

Asteroids::Asteroids()
{

}

Asteroids::~Asteroids()
{
	asteroid_body_->GetWorld()->DestroyBody(asteroid_body_);
}




