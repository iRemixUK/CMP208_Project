#include "Bullet.h"

void Bullet::Init(b2World* world_, float xPosition, PrimitiveBuilder* primitive_builder_)
{
	// Create a physics body for the player
	bullet_body_def_.type = b2_dynamicBody;
	bullet_body_def_.position = b2Vec2(xPosition, -13.0f);
	bullet_body_def_.userData.pointer = reinterpret_cast<uintptr_t>(this);

	bullet_body_ = world_->CreateBody(&bullet_body_def_);

	// Create the shape for the player
	b2PolygonShape bullet_shape;
	bullet_shape.SetAsBox(2, 4);

	// Create the fixture
	b2FixtureDef bullet_fixture_def;
	bullet_fixture_def.shape = &bullet_shape;
	bullet_fixture_def.density = 0.1;

	// Create the fixture on the rigid body
	bullet_body_->CreateFixture(&bullet_fixture_def);

	bullet.set_mesh(primitive_builder_->CreateBoxMesh(gef::Vector4(2, 4, 0)));
}

void Bullet::Update()
{
	// Update simulation
	UpdateFromSimulation(bullet_body_);

	gef::Matrix44 bullet_transform;
	bullet_transform.SetIdentity();
	bullet_transform.SetTranslation(gef::Vector4(bullet_body_->GetPosition().x,
		bullet_body_->GetPosition().y,
		0.0f));
	bullet.set_transform(bullet_transform);

	bullet_body_->ApplyForceToCenter(b2Vec2(0.0f, 60.f), true);
}

void Bullet::Render(gef::Renderer3D* renderer_3d, PrimitiveBuilder* primitive_builder_)
{
	renderer_3d->set_override_material(&primitive_builder_->red_material());
	renderer_3d->DrawMesh(bullet);
	renderer_3d->set_override_material(NULL);
}

Bullet::Bullet()
{

}

Bullet::~Bullet()
{
	bullet_body_->GetWorld()->DestroyBody(bullet_body_);
	
}
