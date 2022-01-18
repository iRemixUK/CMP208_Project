#include "scene_app.h"
#include <system/platform.h>
#include <graphics/sprite_renderer.h>
#include <graphics/font.h>
#include <system/debug_log.h>
#include <graphics/renderer_3d.h>
#include <graphics/mesh.h>
#include <maths/math_utils.h>
#include <input/input_manager.h>
#include <input/sony_controller_input_manager.h>
#include <random>
#include <stdlib.h>     
#include <time.h>
#include <algorithm>
#include <graphics/sprite.h>
#include "load_texture.h"


SceneApp::SceneApp(gef::Platform& platform) :
	Application(platform),
	sprite_renderer_(NULL),
	renderer_3d_(NULL),
	primitive_builder_(NULL),
	font_(NULL),
	world_(NULL),
	scene_assets_(NULL),
	spawnTimer(0.0f),
	game_state_(GameState::None),
	audio_manager_(NULL),
	score(0),
	laser(-1),
	explosion(-1),
	menuScroll(-1),
	menuSelect(-1),
	back(-1),
	voice_id_(-1),
	sample_volume_(1.f),
	sample_pan_(0.0f),
	music_id_(-1),
	music_playing_(false),
	difficulty(false),
	exit(false),
	flash(false)
{
}

void SceneApp::Init()
{
	// Initialise random seed 
	srand(time(NULL));
	sprite_renderer_ = gef::SpriteRenderer::Create(platform_);

	// create the renderer for draw 3D geometry
	renderer_3d_ = gef::Renderer3D::Create(platform_);

	// initialise primitive builder to make create some 3D geometry easier
	primitive_builder_ = new PrimitiveBuilder(platform_);

	InitFont();

	// Create the input manager 
	input_ = gef::InputManager::Create(platform_);
	
	// initialise audio manager
	audio_manager_ = gef::AudioManager::Create();

	// Initialise all samples 
	laser = audio_manager_->LoadSample("laser.wav", platform_);
	explosion = audio_manager_->LoadSample("explosion.wav", platform_);
	menuScroll = audio_manager_->LoadSample("MenuScroll.wav", platform_);
	menuSelect = audio_manager_->LoadSample("MenuSelect.wav", platform_);
	back = audio_manager_->LoadSample("Back.wav", platform_);

	// Initialise music
	//music_id_ = audio_manager_->LoadMusic("music.wav", platform_);

	// Change game state to the initialisation
	ChangeGameState(GameState::Init);

	// Sets the background
	background = CreateTextureFromPNG("Galaxy.png", platform_);
}

void SceneApp::CleanUp()
{
	// destroying the physics world also destroys all the objects within it
	delete world_;
	world_ = NULL;

	// clean up scene assets
	delete scene_assets_;
	scene_assets_ = NULL;

	// Clean up audio
	audio_manager_->UnloadSample(laser);
	audio_manager_->UnloadSample(explosion);
	audio_manager_->UnloadSample(menuScroll);
	audio_manager_->UnloadSample(menuSelect);
	audio_manager_->UnloadMusic();

	// delete audio manager
	delete audio_manager_;
	audio_manager_ = NULL;

	CleanUpFont();

	delete primitive_builder_;
	primitive_builder_ = NULL;

	delete renderer_3d_;
	renderer_3d_ = NULL;

	delete sprite_renderer_;
	sprite_renderer_ = NULL;
}

bool SceneApp::Update(float frame_time)
{
	fps_ = 1.0f / frame_time;

	// call update function for input
	input_->Update();

	// Update the state timer for the init stage
	state_timer_ += frame_time;
	switch (game_state_)
	{
	case GameState::Init:
		InitStateUpdate(frame_time);
		break;
	case GameState::Menu:
		MenuStateUpdate(frame_time);
		break;

	case GameState::Options:
		OptionsUpdate(frame_time);
		break;

	case GameState::Game:
		GameUpdate(frame_time);
		break;

	case GameState::Death:
		DeathUpdate(frame_time);
		break;

	default:
		break;
	}
	
	if (exit == true)
	{
		// Exits application
		return false;
	}

	return true;
}
	
void SceneApp::Render()
{
	// setup camera
	// projection
	float fov = gef::DegToRad(45.0f);
	float aspect_ratio = (float)platform_.width() / (float)platform_.height();
	gef::Matrix44 projection_matrix;
	projection_matrix = platform_.PerspectiveProjectionFov(fov, aspect_ratio, 0.1f, 100.0f);
	renderer_3d_->set_projection_matrix(projection_matrix);

	// view
	gef::Vector4 camera_eye(0.0f, 0.0f, 100.0f);
	gef::Vector4 camera_lookat(0.0f, 0.0f, 0.0f);
	gef::Vector4 camera_up(0.0f, 1.0f, 0.0f);
	gef::Matrix44 view_matrix;
	view_matrix.LookAt(camera_eye, camera_lookat, camera_up);
	renderer_3d_->set_view_matrix(view_matrix);

	// draw 3d geometry
	renderer_3d_->Begin();
	sprite_renderer_->Begin(false);
	switch (game_state_)
	{
	case GameState::Init:
		InitStateRender();
		break;

	case GameState::Game:
		GameRender();
		break;
	
	case GameState::Options:
		OptionsRender();
		break;
	case GameState::Menu:
		MenuStateRender();
		break;

	case GameState::Death:
		DeathRender();
		break;

	default:
		break;
	}
	sprite_renderer_->End();
	renderer_3d_->End();
}

void SceneApp::InitPlayer()
{ 
	// Load in model for the player
	const char* scene_asset_filename = "spaceship3.scn";
	scene_assets_ = LoadSceneAssets(platform_, scene_asset_filename);

	// Sets the mesh to the loaded in model
	gef::Mesh* playerMesh = GetMeshFromSceneAssets(scene_assets_);

	//Initialises the player and sets the object type
	player_.Init(world_, 1, 1);
	player_.SetType(GameObjectType::Player);

	if (scene_assets_)
	{
		// Sets the mesh of the player
		player_.set_mesh(playerMesh);
	}
	else
	{
		gef::DebugOut("Scene file %s failed to load\n", scene_asset_filename);
	}
}

void SceneApp::InitEnviroment()
{
	// Sets the enviroment up so the player can't leave the scene
	enviroment_.Init(primitive_builder_, world_);
	enviroment_.SetType(GameObjectType::enviroment);
}

void SceneApp::InitFont()
{
	font_ = new gef::Font(platform_);
	font_->Load("comic_sans");
}

void SceneApp::CleanUpFont()
{
	delete font_;
	font_ = NULL;
}

void SceneApp::DrawHUD()
{
	if(font_)
	{
		// display frame rate
		font_->RenderText(sprite_renderer_, gef::Vector4(850.0f, 510.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "FPS: %.1f", fps_);
		
		// Display Player health
		font_->RenderText(
			sprite_renderer_,						// sprite renderer to draw the letters
			gef::Vector4(0.0f, 510.0f, -0.9f),		// position on screen
			1.0f,									// scale
			0xffffffff,								// colour ABGR
			gef::TJ_LEFT,							// justification
			"Health: %i",						// string of text to render
			player_.ReturnHealth()						// any variables used in formatted text string http://www.cplusplus.com/reference/cstdio/printf/
			
		);
		// Display score
		font_->RenderText(sprite_renderer_, gef::Vector4(0.0f, 0.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "Score: %.1i", score);
	}
}

void SceneApp::SetupLights()
{
	// grab the data for the default shader used for rendering 3D geometry
	gef::Default3DShaderData& default_shader_data = renderer_3d_->default_shader_data();

	// set the ambient light
	default_shader_data.set_ambient_light_colour(gef::Colour(0.25f, 0.25f, 0.25f, 1.0f));

	// add a point light that is almost white, but with a blue tinge
	// the position of the light is set far away so it acts light a directional light
	gef::PointLight default_point_light;
	default_point_light.set_colour(gef::Colour(0.7f, 0.7f, 1.0f, 1.0f));
	default_point_light.set_position(gef::Vector4(-500.0f, 400.0f, 700.0f));
	default_shader_data.AddPointLight(default_point_light);
}

float SceneApp::RandomFloatZeroToOne()
{
	// Generates a random float between zero and one
	return rand() / (RAND_MAX + 1.0f);
}

float SceneApp::RandomFloat(float min, float max)
{
	// Generates a random number between the two floats that are passed in
	return min + RandomFloatZeroToOne() * (max - min);
}

void SceneApp::DeleteAsteroids()
{
	// Deletes all asteroids that have been marked for deletion
	std::set<Asteroids*>::iterator it = markedAsteroids.begin();
	std::set<Asteroids*>::iterator end = markedAsteroids.end();

	for (; it != end; ++it)
	{
		// Delete asteroid
		Asteroids* dying = *it;
		delete dying;

		std::vector<Asteroids*>::iterator it = std::find(asteroids.begin(), asteroids.end(), dying);
		if (it != asteroids.end())
		{
			// Remove the dead asteroid from the container of alive asteroid
			asteroids.erase(it);
		}
	}
	// Clears the container of marked asteroid
	markedAsteroids.clear();
}

void SceneApp::DeleteBullets()
{
	// Deletes all bullets that have been marked for deletion
	std::set<Bullet*>::iterator it = markedBullets.begin();
	std::set<Bullet*>::iterator end = markedBullets.end();

	for (; it != end; ++it)
	{
		// Delete bullet
		Bullet* dying = *it;
		delete dying;

		std::vector<Bullet*>::iterator it = std::find(bullets.begin(), bullets.end(), dying);
		if (it != bullets.end())
		{
			// Remove the dead bullet from the container of alive bullets
			bullets.erase(it);
		}
	}
	// Clears the container of marked bullets 
	markedBullets.clear();
}

// Done
void SceneApp::GameInit()
{
	// Initialise random seed 
	srand(time(NULL));

	// Light setup
	SetupLights();

	// initialise the physics world
	b2Vec2 gravity(0.0f, 0.f);
	world_ = new b2World(gravity);

	const char* scene_asset_filename = "asteroid2.scn";
	scene_assets_ = LoadSceneAssets(platform_, scene_asset_filename);
	asteroidMesh = GetMeshFromSceneAssets(scene_assets_);

	// Sets the background
	background = CreateTextureFromPNG("Galaxy.png", platform_);

	// Initialises the player
	InitPlayer();

	// Initialises the enviroment 
	InitEnviroment();

	// Sets the player health based on the difficulty and also changes the spawn time for the asteroids
	if (difficulty == false)
	{
		player_.SetHealth(20);
		spawnTimeLimit = 2.f;
	}
	if (difficulty == true)
	{
		player_.SetHealth(10);
		spawnTimeLimit = 1.f;
	}


}

void SceneApp::GameRelease()
{	
	// clean up scene assets
	delete scene_assets_;
	scene_assets_ = NULL;

	// Mark all asteroids for deletion
	for (auto obj : asteroids)
	{
		markedAsteroids.insert(obj);
	}

	// Mark all bullets for deletion
	for (auto obj : bullets)
	{
		markedBullets.insert(obj);
	}

	DeleteAsteroids();
	DeleteBullets();
} 

void SceneApp::GameUpdate(float frame_time)
{
	//flash = false;
	// Update the spawn timer for the asteroids
	spawnTimer += frame_time;
	
	VolumeInfo_.volume = sample_volume_;
	VolumeInfo_.pan = sample_pan_;

	// update physics world
	float timeStep = 1.0f / 60.0f;
	int32 velocityIterations = 6;
	int32 positionIterations = 2;
	world_->Step(timeStep, velocityIterations, positionIterations);

	// setup controller manager
	gef::SonyControllerInputManager* controllerManager = input_->controller_input();

	if (controllerManager)
	{
		const gef::SonyController* controller = controllerManager->GetController(0);

		// Deals with input relating to the player
		player_.Input(controller);

		if (controller->buttons_down() & gef_SONY_CTRL_START)
		{
			// Ends the game when pressed
			ChangeGameState(GameState::Death);
		}

		if (controller->buttons_released() & gef_SONY_CTRL_R1)
		{
			voice_id_ = audio_manager_->PlaySample(laser, false);
			bullet_ = new Bullet;
			bullet_->Init(world_, player_.Position(), primitive_builder_);
			bullet_->SetType(GameObjectType::bullet);
			bullets.push_back(bullet_);
		}
	}

	if (spawnTimer > spawnTimeLimit)
	{
		// Spawns a new asteroid every 2 seconds
		asteroid = new Asteroids;
		asteroid->Init(world_, asteroidMesh, RandomFloat(-50, 50));
		asteroid->SetType(GameObjectType::enemy);
		asteroids.push_back(asteroid);
		spawnTimer = 0;
	}

	if (!asteroids.empty())
	{
		for (int i = 0; i < asteroids.size(); i++)
		{
			// Iterate through alive asteroids and update them
			asteroids[i]->Update();

			if (asteroids[i]->PositionY() < -50.f)
			{
				// Mark Asteroids for deletion when they have left the screen
				markedAsteroids.insert(asteroids[i]);
			}
		}
	}

	if (!bullets.empty())
	{
		for (int i = 0; i < bullets.size(); i++)
		{
			// Iterate through alive bullets and update them
			bullets[i]->Update();

			if (bullets[i]->PositionY() > 40.f)
			{
				// Mark bullets for deletion when they have left the screen
				markedBullets.insert(bullets[i]);
			}
		}
	}

	// Updates the player
	player_.Update();

	// collision detection
	// get the head of the contact list
	b2Contact* contact = world_->GetContactList();

	// get contact count
	int contact_count = world_->GetContactCount();

	for (int contact_num = 0; contact_num < contact_count; ++contact_num)
	{
		if (contact->IsTouching())
		{
			// get the colliding bodies
			b2Body* bodyA = contact->GetFixtureA()->GetBody();
			b2Body* bodyB = contact->GetFixtureB()->GetBody();

			// DO COLLISION RESPONSE HERE
			Asteroids* objectA = reinterpret_cast<Asteroids*>(bodyA->GetUserData().pointer);
			Asteroids* objectB = reinterpret_cast<Asteroids*>(bodyB->GetUserData().pointer);

			if (objectA)
			{
				if (objectA->GetType() == GameObjectType::Player && objectB->GetType() == GameObjectType::enemy)
				{
					// Decrease the player's health if it has collided with an asteroid
					player_.DecrementHealth();
					flash = true;
				}

				if (objectB->GetType() == GameObjectType::bullet && objectA->GetType() == GameObjectType::enemy)
				{
					// Mark asteroid for deletion if it has collided with a bullet
					markedAsteroids.insert(objectA);
					score += 10;
					voice_id_ = audio_manager_->PlaySample(explosion, false);
				}
			}

			if (objectB)
			{
				if (objectA->GetType() == GameObjectType::Player && objectB->GetType() == GameObjectType::enemy ||
					objectA->GetType() == GameObjectType::enemy && objectB->GetType() == GameObjectType::Player ||
					objectA->GetType() == GameObjectType::bullet && objectB->GetType() == GameObjectType::enemy)
				{
					// Mark asteroid for deletion if it has collided with a bullet
					markedAsteroids.insert(objectB);
					score += 10;

					voice_id_ = audio_manager_->PlaySample(explosion, false);
				}
			}
		}

		// Get next contact point
		contact = contact->GetNext();
	}

	DeleteAsteroids(); // Deletes the asteroids that have been marked for deletion
	DeleteBullets(); // Deletes the bullets that have been marked for deletion

	if (player_.ReturnHealth() == 0)
	{
		ChangeGameState(GameState::Death);
	}
}

void SceneApp::GameRender()
{
	// Draw background sprite
	Background.set_texture(background);
	Background.set_position(gef::Vector4(0.f, 0.f, 500.f));
	Background.set_height(1100.0f);
	Background.set_width(1950.0f);
	sprite_renderer_->DrawSprite(Background);

	if (flash == true)
	{
		// Screen flashes red if the player is hit by an asteroid
		gef::Sprite Flash;
		Flash.set_colour(0xFF0000FF); // Set the colour to red
		Flash.set_position(gef::Vector4(0.f, 0.f, -500.f));
		Flash.set_height(1100.0f);
		Flash.set_width(1950.0f);
		sprite_renderer_->DrawSprite(Flash);
	}

	// Draw hud
	DrawHUD();

	// draw enviroment
	enviroment_.Render(renderer_3d_);

	// draw player
	renderer_3d_->DrawMesh(player_); // Draws the player 
	renderer_3d_->set_override_material(NULL); // Resets the material

	// draw asteroids
	if (!asteroids.empty())
	{
		for (auto obj : asteroids)
		{
			obj->Render(renderer_3d_, primitive_builder_);
		}
	}

	// Draw bullets
	if (!bullets.empty())
	{
		for (auto obj : bullets)
		{
			obj->Render(renderer_3d_, primitive_builder_);
		}
	}
	flash = false; // Set flash to false so the screen only flashes for a frame 
}

void SceneApp::InitInit()
{
	splash_art = CreateTextureFromPNG("Splash.png", platform_);

	// Initialise menu screen
	menu1 = CreateTextureFromPNG("Menu1.png", platform_);
	menu2 = CreateTextureFromPNG("Menu2.png", platform_);
	menu3 = CreateTextureFromPNG("Menu3.png", platform_);
	how_to_play = CreateTextureFromPNG("HowToPlay.png", platform_);

	// Initialise option screen
	option1 = CreateTextureFromPNG("Options1.png", platform_);
	option2 = CreateTextureFromPNG("Options2.png", platform_);

	// Loading these here ensure that these PNG's are only loaded once into memory ensuring memory leaks can't happen
	// This also ensures performance is good as everything that is needed is loaded at the start of the program, 
	// so no unexpected FPS drops happen when PNGs are loaded into memory
}

void SceneApp::InitStateUpdate(float frame_time)
{
	if (state_timer_ > 3.0f)
	{
		// After 3 seconds switch to the menu game state
		ChangeGameState(GameState::Menu);
	}
}

void SceneApp::InitStateRender()
{
	// Render splash screen
	gef::Sprite Splash;
	Splash.set_texture(splash_art);
	Splash.set_position(gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f, 500.f));
	Splash.set_height(550.f);
	Splash.set_width(970.f);
	sprite_renderer_->DrawSprite(Splash);
}

void SceneApp::InitRelease()
{
	delete splash_art;
	splash_art = NULL;
}

void SceneApp::MenuStateInit()
{
	MenuState = 0;
}

void SceneApp::MenuStateUpdate(float frame_time)
{
	// Menu state which changes on user input
	if (MenuState == 0)
	{
		// Sets it to play
		Menu.set_texture(menu1);
	}
	
	if (MenuState == 1)
	{
		// Sets it to how to play
		Menu.set_texture(menu2);
	}
	
	if (MenuState == 2)
	{
		// Sets it to exit
		Menu.set_texture(menu3);
	}

	if (MenuState == 3)
	{
		// Sets it to how to play screen
		Menu.set_texture(how_to_play);
	}

	// setup controller manager
	gef::SonyControllerInputManager* controllerManager = input_->controller_input();

	if (controllerManager)
	{
		const gef::SonyController* controller = controllerManager->GetController(0);

		if (controller->buttons_released() & gef_SONY_CTRL_UP)
		{
			voice_id_ = audio_manager_->PlaySample(menuScroll, false);
			MenuState = MenuState - 1;
			if (MenuState < 0)
			{
				MenuState = 0;
			}
		}

		if (controller->buttons_released() & gef_SONY_CTRL_DOWN)
		{
			voice_id_ = audio_manager_->PlaySample(menuScroll, false);
			MenuState = MenuState + 1;
			if (MenuState > 2)
			{
				MenuState = 2;
			};
		}

		if (controller->buttons_released() & gef_SONY_CTRL_CROSS && MenuState == 0)
		{
			voice_id_ = audio_manager_->PlaySample(menuSelect, false);
			ChangeGameState(GameState::Options);
		}

		if (controller->buttons_released() & gef_SONY_CTRL_CROSS && MenuState == 1)
		{
			voice_id_ = audio_manager_->PlaySample(menuSelect, false);
			MenuState = 3;
		}

		if (controller->buttons_released() & gef_SONY_CTRL_CROSS && MenuState == 2)
		{
			exit = true;
		}
		
		if (controller->buttons_released() & gef_SONY_CTRL_CIRCLE && MenuState == 3)
		{
			voice_id_ = audio_manager_->PlaySample(back, false);
			MenuState = 1;
		}
	}

}

void SceneApp::MenuStateRender()
{
	// Render menu screen
	Menu.set_position(gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f, 500.f));
	Menu.set_height(550.f);
	Menu.set_width(970.f);
	sprite_renderer_->DrawSprite(Menu);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SceneApp::MenuStateRelease()
{

}

void SceneApp::OptionsInit()
{
	OptionState = 0;
}

void SceneApp::OptionsUpdate(float frame_time)
{
	// Menu state which changes on user input
	if (OptionState == 0)
	{
		// Sets it to play
		Option.set_texture(option1);
	}

	if (OptionState == 1)
	{
		// Sets it to how to play
		Option.set_texture(option2);
	}

	// setup controller manager
	gef::SonyControllerInputManager* controllerManager = input_->controller_input();

	if (controllerManager)
	{
		const gef::SonyController* controller = controllerManager->GetController(0);

		if (controller->buttons_released() & gef_SONY_CTRL_LEFT)
		{
			voice_id_ = audio_manager_->PlaySample(menuScroll, false);
			OptionState = OptionState - 1;
			if (OptionState < 0)
			{
				OptionState = 0;
			}
		}

		if (controller->buttons_released() & gef_SONY_CTRL_RIGHT)
		{
			voice_id_ = audio_manager_->PlaySample(menuScroll, false);
			OptionState = OptionState + 1;
			if (OptionState > 1)
			{
				OptionState = 1;
			};
		}

		if (controller->buttons_released() & gef_SONY_CTRL_CROSS && OptionState == 0)
		{
			voice_id_ = audio_manager_->PlaySample(menuSelect, false);
			difficulty = false;
			ChangeGameState(GameState::Game);
		}

		if (controller->buttons_released() & gef_SONY_CTRL_CROSS && OptionState == 1)
		{
			voice_id_ = audio_manager_->PlaySample(menuSelect, false);
			difficulty = true;
			ChangeGameState(GameState::Game);
		}

		if (controller->buttons_released() & gef_SONY_CTRL_CIRCLE)
		{
			voice_id_ = audio_manager_->PlaySample(back, false);
			OptionState = 0;
			ChangeGameState(GameState::Menu);
		}
	}
}

void SceneApp::OptionsRender()
{
	// Render options screen
	Option.set_position(gef::Vector4(platform_.width() * 0.5f, platform_.height() * 0.5f, 500.f));
	Option.set_height(550.f);
	Option.set_width(970.f);
	sprite_renderer_->DrawSprite(Option);
}

void SceneApp::OptionsRelease()
{

}

void SceneApp::DeathInit()
{

}

void SceneApp::DeathUpdate(float frame_time)
{
	if (state_timer_ > 5.0f)
	{
		ChangeGameState(GameState::Menu);
	}
}

void SceneApp::DeathRender()
{
	// Draw background sprite
	Background.set_texture(background);
	Background.set_position(gef::Vector4(0.f, 0.f, 500.f));
	Background.set_height(1100.0f);
	Background.set_width(1950.0f);
	sprite_renderer_->DrawSprite(Background);

	font_->RenderText(sprite_renderer_, gef::Vector4(285.f, 255.0f, -0.9f), 1.0f, 0xffffffff, gef::TJ_LEFT, "Game Over! Your final score was %.1i", score);
}

void SceneApp::DeathRelease()
{
	// Delete background assest
	delete background;
	background = NULL;

	// Reset the score
	score = 0;
}

void SceneApp::ChangeGameState(GameState new_state)
{
	// Clean up old state
	switch (game_state_)
	{
	case GameState::Init:
		// Clean up init state
		InitRelease();
		break;

	case GameState::Menu:
		MenuStateRelease();
		break;

	case GameState::Game:
		GameRelease();
		break;

	case GameState::Options:
		OptionsRelease();
		break;

	case GameState::Death:
		DeathRelease();
		break;

	case GameState::None:
	default:
		break;
	}

	game_state_ = new_state;
	state_timer_ = 0.0f;

	// Init new state
	switch (game_state_)
	{
	case GameState::Init:
		// Init state
		InitInit();
		break;

	case GameState::Menu:
		MenuStateInit();
		break;

	case GameState::Options:
		OptionsInit();
		break;

	case GameState::Game:
		GameInit();
		break;

	case GameState::Death:
		DeathInit();
		break;

	case GameState::None:
	default:
		break;
	}
}

void SceneApp::RenderStateName(char* name)
{

}

gef::Scene* SceneApp::LoadSceneAssets(gef::Platform& platform, const char* filename)
{
	gef::Scene* scene = new gef::Scene();

	if (scene->ReadSceneFromFile(platform, filename))
	{
		// if scene file loads successful
		// create material and mesh resources from the scene data
		scene->CreateMaterials(platform);
		scene->CreateMeshes(platform);
	}
	else
	{
		delete scene;
		scene = NULL;
	}

	return scene;
}

gef::Mesh* SceneApp::GetMeshFromSceneAssets(gef::Scene* scene)
{
	gef::Mesh* mesh = NULL;

	// if the scene data contains at least one mesh
	// return the first mesh
	if (scene && scene->meshes.size() > 0)
		mesh = scene->meshes.front();

	return mesh;
}
