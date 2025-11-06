#include "pch.h"
#include "Application.h"

#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Events/Event.h"
#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Audio/AudioManager.h"
#include "CoreSystems/Scene/Scene.h"
#include "CoreSystems/Scene/sCameraController.h"

// Serialization
#include "CoreSystems/Serialization/sSerialization.h"
#include "LayeredSystems/LevelEditor/Editor.h"

#include "ECS/Controller.h"
#include "ECS/sMetaData.h"

#include "Core.h"

// Assets
#include "CoreSystems/Assets/sPath.h"
#include "CoreSystems/Assets/sLoader.h"
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Assets/sAssetCompiler.h"
#include "CoreSystems/Path/Path.h"

// Systems
#include "Systems/Physics/sysPhysics.h"
#include "Systems/AI/sysAI.h" 
#include "Systems/Animation/sysAnimation.h" 
#include "Systems/Scripting/sysScripting.h" 
#include "Systems/Logic/sysLogic.h"
#include "Systems/Audio/sysAudio.h"
#include "Systems/Collision/sBVHSystem.h"
#include "Systems/Scripting/GameScriptingSystem.h"

#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"

#include "CoreSystems/Renderer/text.h"

namespace PAIN {

	Application::Application() {
		//Create default services
		services = std::make_shared<Services>();
	}

	Application::~Application() {
		//Destroy top down
		for (auto it = layer_stack.rbegin(); it != layer_stack.rend(); ++it) {

			//On detach
			(*it).lock()->onDetach();
			(*it).lock()->services = nullptr;
		}
		layer_stack.clear();

		//Destroy to core top down
		for (auto it = core_stack.rbegin(); it != core_stack.rend(); ++it) {

			//On detach
			(*it).lock()->onDetach();
			(*it).lock()->services = nullptr;
		}
		core_stack.clear();
	}

	template<typename T>
	void Application::addCoreSystem(std::shared_ptr<T> core_system) {
		core_system->services = services;
		core_system->onAttach();
		services->set<T>(core_system);
		core_stack.push_back(services->get<T>());
	}

	template<typename T>
	void Application::addLayerSystem(std::shared_ptr<T> layer_system) {
		layer_system->services = services;
		layer_system->onAttach();
		services->set<T>(layer_system);
		layer_stack.push_back(services->get<T>());
	}

	void Application::Init(void* app) {

		//Initialize logger
		PAIN::Log::Init();
		PN_CORE_INFO("Initialized Log!");

		auto app_window = std::shared_ptr<Window::Window>(Window::Window::create(app));
		app_window->registerCallbacks(this);
		addCoreSystem(app_window);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// Match viewport to window size
		auto window = services->get<Window::Window>();

		//Create path service
		services->set<Path::Path>(std::shared_ptr<Path::Path>(Path::Path::create(app)));
		services->get<Path::Path>()->logVirtualPaths();

		//Create asset service
		addCoreSystem(std::make_shared<Assets::Manager>());

		//Create and add the AudioManager to the core systems
		auto app_audio = std::shared_ptr<Audio::Audio>(Audio::Audio::create(app));
		addCoreSystem(app_audio);

		// dependency injection
		TextRenderer::init(services);
		//Audio testing.
#ifdef PN_PLATFORM_WINDOWS
        auto asset_path = services->get<Path::Path>()->resolvePath("game_assets://audio/music/Boss_Music.wav");
		PN_CORE_INFO(asset_path);
		app_audio->loadSound(asset_path, true, false, false);
        //app_audio->play(asset_path);
#else
        app_audio->loadSound("file:///android_asset/game/audio/music/Boss_Music.wav", true, false, false);
        app_audio->play("file:///android_asset/game/audio/music/Boss_Music.wav");
#endif

		//Push other core systems into the stack
		addCoreSystem(std::make_shared<ECS::Controller>(services));
		addCoreSystem(std::make_shared<MetaData::Service>());

		// Add Serialization
		addCoreSystem(std::make_shared<Serialization::Service>());

		// Physics system cross platform
		services->get<ECS::Controller>()->registerSystem<Physics::System>();
		
#ifdef PN_PLATFORM_WINDOWS	
		// Physics system not cross platform yet
		services->get<ECS::Controller>()->registerSystem<Physics::System>();

		services->get<ECS::Controller>()->registerSystem<AI::System>();
		services->get<ECS::Controller>()->registerSystem<Animation::System>();
		services->get<ECS::Controller>()->registerSystem<Scripting::System>();
		services->get<ECS::Controller>()->registerSystem<Logic::System>();
		services->get<ECS::Controller>()->registerSystem<Audio::System>();
#endif
		services->get<ECS::Controller>()->registerSystem<sBVHSystem>();

		// Register components here
		services->get<ECS::Controller>()->registerAllComponents();


		// Windows only have paths, andriods have to use AASettmanager
#ifdef PN_PLATFORM_WINDOWS
		addCoreSystem(std::make_shared<Path::Service>());
		services->get<Path::Service>()->init("assets/Config.json");
		addCoreSystem(std::make_shared<Assets::Service>());
		addCoreSystem(std::make_shared<Compiler::Service>());
#endif
		// Scenes
		addCoreSystem(std::make_shared<Scene>());

		// Camera System
		addCoreSystem(std::make_shared<sCameraController>());

		// Renderer
		addCoreSystem(std::make_shared<sRenderer>());

		// game scripting system
		addCoreSystem<GameScriptingSystem>(std::make_shared<GameScriptingSystem>());

		//Editor only added when debug mode
#ifdef _DEBUG
		// !NOTE: IMGUI eats events
		auto editor = std::make_shared<Editor::Editor>(app_window->getNativeWindow());
		addLayerSystem(editor);
#endif

#ifdef _DEBUG  // THIS BLOCK WAS FOR TESTING LUA
		PN_CORE_INFO("Testing Lua on Debug");

		auto ecs_ptr = services->get<ECS::Controller>();
		auto meta_ptr = services->get<MetaData::Service>();
		auto scripting_ptr = services->get<GameScriptingSystem>();


		if (ecs_ptr && meta_ptr && scripting_ptr) {
			auto& ecs = *ecs_ptr;
			auto& meta = *meta_ptr;
			auto& scripting = *scripting_ptr;

			// Create test player entity
			auto player = ecs.createEntity();
			meta.setEntityName(player, "Player");

			// Add Transform component (minimal for testing)
			ecs.addEntityComponent<Transform>(player, Transform{});

			// Attach Lua script
			int entityId = static_cast<int>(entt::to_integral(player));
			scripting.attachScript(entityId, "game/scripts/PlayerController.lua");
			PN_CORE_INFO("Test player spawned with entity ID: {}", entityId);
		} 
		else {
			PN_CORE_ERROR("Failed to spawn test player - services not available");
		}
#else
		PN_CORE_INFO("Testing Lua on Release");

		auto ecs_ptr = services->get<ECS::Controller>();
		auto meta_ptr = services->get<MetaData::Service>();
		auto scripting_ptr = services->get<GameScriptingSystem>();


		if (ecs_ptr && meta_ptr && scripting_ptr) {
			auto& ecs = *ecs_ptr;
			auto& meta = *meta_ptr;
			auto& scripting = *scripting_ptr;

			// Create test player entity
			auto player = ecs.createEntity();
			meta.setEntityName(player, "Player");

			// Add Transform component (minimal for testing)
			ecs.addEntityComponent<Transform>(player, Transform{});

			// Attach Lua script
			int entityId = static_cast<int>(entt::to_integral(player));
			scripting.attachScript(entityId, "game/scripts/PlayerController.lua");
			PN_CORE_INFO("Test player spawned with entity ID: {}", entityId);
		}
		else {
			PN_CORE_ERROR("Failed to spawn test player - services not available");
		}
#endif

		//Mark engine as ready
		b_app_running = true;
	}

	void Application::Run() {

	//Set last time
	last_time = std::chrono::steady_clock::now();

	//Application loop
	while (b_app_running) {

		//Poll events
		auto window = services->get<Window::Window>();
		if (window) window->pollEvents();

		//Drain all events in queue
		drainEventQueue();

		//Update delta time
		auto now = std::chrono::steady_clock::now();
		timing.dt = std::chrono::duration<float>(now - last_time).count();
		last_time = now;

		auto fps = static_cast<int>(1.f / timing.dt);
#ifdef PN_PLATFORM_WINDOWS
		static float avgFps = 0.f;
		static float timeSinceLastUpdate = 0.0f;

		avgFps = avgFps * 0.95f + fps * 0.05f;
		timeSinceLastUpdate += timing.dt;

		if (timeSinceLastUpdate >= 0.5f) {
			timeSinceLastUpdate = 0.0f;
			auto window = services->get<Window::Window>();
			std::string title = "Pain Engine - FPS: " + std::to_string(static_cast<int>(avgFps));
			glfwSetWindowTitle(
				reinterpret_cast<GLFWwindow*>(window->getNativeWindow()),
				title.c_str()
			);
		}
#endif
	
#ifdef _DEBUG
		if (services->get<Editor::Editor>()->isPaused()) {
			timing.dt = 0.0f;
			services->get<Audio::Audio>()->pauseAll();
		}
		else {
			services->get<Audio::Audio>()->resumeAll();
		}
#endif

		//Accumulate for fixed updates (use scaled time)
		accumulator += timing.dt;  // Changed from timing.dt


		//Skip all other systems when window is not active
		if (!services->get<Window::Window>()->getActive()) {
			services->get<Window::Window>()->swapBuffers();
			continue;
		}


		//Update fixed delta
		int steps = 0;
		while (accumulator >= timing.fixed_dt && steps < MAX_STEPS) {

			//Update all core systems
			for (auto& core : core_stack) {
				//if (auto core_ptr = core.lock()) core_ptr->onFixedUpdate(timing);

				auto core_ptr = core.lock();
				if (core_ptr) core_ptr->onFixedUpdate(timing);
			}

			//Update all layered systems
			for (auto& layer : layer_stack) {
				//if (auto layer_ptr = core.lock()) layer_ptr->onFixedUpdate(timing);

				auto layer_ptr = layer.lock();
				if (layer_ptr) layer_ptr->onFixedUpdate(timing);

			}

			accumulator -= timing.fixed_dt;
			++steps;
		}

		//Update timing variables
		timing.steps_this_frame = steps;
		timing.alpha = static_cast<float>(accumulator / timing.fixed_dt);

		// clear errors before next rendering loop

		while (glGetError());

		//Update all core systems
		for (auto& core : core_stack) {
			auto core_ptr = core.lock();
            if (core_ptr) core_ptr->onUpdate(timing);
		}

		//Update all layered systems
		for (auto& layer : layer_stack) {
			auto layer_ptr = layer.lock();
            if (layer_ptr) layer_ptr->onUpdate(timing);
		}

		//Swap buffer
		services->get<Window::Window>()->swapBuffers();
	};
}


	void Application::terminate() {
		b_app_running = false;
	}

	void Application::dispatchEventsForward(Event::Event& e) {
		//Boolean flag for propogation
		bool handled = false;

		//Dispatch to core from bottom up
		for (auto it = core_stack.begin(); it != core_stack.end(); ++it) {

			auto layer_ptr = it->lock();
        	if (!layer_ptr) continue; // skip null weak_ptr

			//Dispatch event down layers
			(*it).lock()->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}

		//Check if handled
		if (handled) return;

		//Dispatch to layer bottom up
		for (auto it = layer_stack.begin(); it != layer_stack.end(); ++it) {

			auto core_ptr = it->lock();
        	if (!core_ptr) continue; // skip null weak_ptr

			//Dispatch event down layers
			(*it).lock()->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}
	}

	void Application::dispatchEventsReversed(Event::Event& e) {
		//Boolean flag for propogation
		bool handled = false;

		//Dispatch to layer top down
		for (auto it = layer_stack.rbegin(); it != layer_stack.rend(); ++it) {
			if (auto layer_ptr = it->lock()) {
           		layer_ptr->onEvent(e);
           		if (e.checkHandled()) return;
       		}
		}

		//Check if handled
		if (handled) return;

		//Dispatch to core top down
		for (auto it = core_stack.rbegin(); it != core_stack.rend(); ++it) {

			//Dispatch event down layers
			if (auto core_ptr = it->lock()) {
            	core_ptr->onEvent(e);
            	if (e.checkHandled()) return;
	       	}
		}
	}

	void Application::dispatchEvent(Event::Event& e) {

		//Check event type
		if (e.isInCategory(Event::Input)) {

			//Dispatch event in reverse order
			dispatchEventsReversed(e);
		}
		else {
			//Dispatch event in order
			dispatchEventsForward(e);
		}
	}

	void Application::pushEventQueue(std::shared_ptr<Event::Event> e) {
		event_queue.push(e);
	}	

	void Application::drainEventQueue() {

		//Handle all events in queue
		while (!event_queue.empty()) {

			//Handle event and pop from queue
			dispatchEvent(*event_queue.front());
			event_queue.pop();
		}
	}
}