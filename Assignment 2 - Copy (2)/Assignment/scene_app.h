#ifndef _SCENE_APP_H
#define _SCENE_APP_H

#include <system/application.h>
#include <maths/vector2.h>
#include "primitive_builder.h"
#include <graphics/mesh_instance.h>
#include "graphics/scene.h"
#include <box2d/Box2D.h>
#include "game_object.h"
#include "player.h"
#include "Enviroment.h"
#include "Asteroids.h"
#include <vector>
#include <set>
#include "Bullet.h"
#include <graphics/sprite.h>
#include <audio/audio_manager.h>

// FRAMEWORK FORWARD DECLARATIONS
namespace gef
{
	class Platform;
	class SpriteRenderer;
	class Font;
	class InputManager;
	class Renderer3D;
}

// Game states
enum GameState {
	None,
	Init,
	Menu,
	Options,
	Game,
	Pause,
	Death,
	Exit
};

// Object types
enum GameObjectType {
	enviroment, 
	Player, 
	enemy,
	bullet 
};

class SceneApp : public gef::Application
{
public:
	SceneApp(gef::Platform& platform);
	void Init();
	void CleanUp();
	bool Update(float frame_time);
	void Render();
private:
	void InitPlayer();
	void InitEnviroment();
	void InitFont();
	void CleanUpFont();
	void DrawHUD();
	void SetupLights();
	float RandomFloatZeroToOne();
	float RandomFloat(float min, float max);
	void DeleteAsteroids();
	void DeleteBullets();

	// Game state stuff
	void GameInit();
	void GameRelease();
	void GameUpdate(float frame_time);
	void GameRender();

	void InitInit();
	void InitStateUpdate(float frame_time);
	void InitStateRender();
	void InitRelease();

	void MenuStateInit();
	void MenuStateUpdate(float frame_time);
	void MenuStateRender();
	void MenuStateRelease();

	void OptionsInit();
	void OptionsUpdate(float frame_time);
	void OptionsRender();
	void OptionsRelease();

	void DeathInit();
	void DeathUpdate(float frame_time);
	void DeathRender();
	void DeathRelease();

	void ChangeGameState(GameState new_state);

	void RenderStateName(char* name);

	gef::Scene* LoadSceneAssets(gef::Platform& platform, const char* filename);
	gef::Mesh* GetMeshFromSceneAssets(gef::Scene* scene);
	gef::MeshInstance mesh_instance_;
	gef::Scene* scene_assets_;
    
	gef::SpriteRenderer* sprite_renderer_;
	gef::Font* font_;
	gef::Renderer3D* renderer_3d_;

	PrimitiveBuilder* primitive_builder_;

	// Audio stuff
	gef::AudioManager* audio_manager_;
	gef::VolumeInfo VolumeInfo_;
	int laser;
	int explosion;
	int menuScroll;
	int menuSelect;
	int back;

	int voice_id_;
	int music_id_;
	bool music_playing_;
	float sample_volume_;
	float sample_pan_;

	// create the physics world
	b2World* world_;

	// player variables
	player player_;
	gef::Mesh* player_mesh_;

	// Game Background
	gef::Texture* background;
	gef::Texture* splash_art;
	gef::Texture* menu1;
	gef::Texture* menu2;
	gef::Texture* menu3;
	gef::Texture* how_to_play;
	gef::Texture* option1;
	gef::Texture* option2;

	gef::Sprite Background;
	gef::Sprite Menu;
	gef::Sprite Option;

	// Asteroid variables
	Asteroids* asteroid;
	std::vector<Asteroids*> asteroids;
	std::set<Asteroids*> markedAsteroids;
	gef::Mesh* asteroidMesh;
	float spawnTimer;
	float spawnTimeLimit;

	// Bullet variables
	Bullet* bullet_;
	std::vector<Bullet*> bullets;
	std::set<Bullet*> markedBullets;

	// Ground variables
	Enviroment enviroment_;
	gef::Mesh* ground_mesh_;

	// Score variable
	int score;

	// Game state variables
	GameState game_state_;
	float state_timer_;

	bool exit;
	bool flash;

	int MenuState; // Menu State
	int OptionState; // Option State

	bool difficulty; // Difficulty of the game, false = easy / true = hard

	gef::InputManager* input_;

	float fps_;
};

#endif // _SCENE_APP_H