#ifndef _GAME_OBJECT_H
#define _GAME_OBJECT_H

#include <graphics/mesh_instance.h>
#include <box2d/Box2D.h>

class GameObject : public gef::MeshInstance
{
public:
	void init();
	void UpdateFromSimulation(const b2Body* body);
	void SetType(int ObjectType) { type_ = ObjectType; };
	int GetType() { return type_; };
	void SetID(int id_) { id = id_; };
	int GetID() { return id; };
	void Kill() { alive = false; };
	bool ReturnStatus() { return alive; };

	
protected:
	int type_;
	int id;
	bool alive;
};

#endif // _GAME_OBJECT_H