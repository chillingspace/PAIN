#include "pch.h"
#include "Application.h"

#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Events/Event.h"
#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Scene/Scene.h"
#include "CoreSystems/Scene/sCameraController.h"
#include "CoreSystems/Prefabs/sPrefab.h"
#include "CoreSystems/EntityTemplate/sEntityTemplate.h"

#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#include "CoreSystems/Renderer/text.h"

// Serialization
#include "CoreSystems/Serialization/sSerialization.h"
#include "LayeredSystems/LevelEditor/Editor.h"

#include "ECS/Controller.h"
#include "ECS/sMetaData.h"

#include "Core.h"

// Assets
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Path/Path.h"





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
		services->set<T>(core_system);
		core_system->services = services;
		core_system->onAttach();
		core_stack.push_back(services->get<T>());
	}

	template<typename T>
	void Application::addLayerSystem(std::shared_ptr<T> layer_system) {
		services->set<T>(layer_system);
		layer_system->services = services;
		layer_system->onAttach();
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

		//Get sound asset
		//auto sound = services->get<Assets::Manager>()->getAsset<Audio::Sound>("game/audio/music/Boss_Music.wav");
		//app_audio->play(sound);

		// dependency injection
		TextRenderer::init(services);

		//Push other core systems into the stack
		addCoreSystem(std::make_shared<ECS::Controller>(services));
		addCoreSystem(std::make_shared<MetaData::Service>());

		//Create entity template and prefab service
		services->set<EntityTemplate::Service>(std::make_shared<EntityTemplate::Service>(services));
		services->set<Prefab::Service>(std::shared_ptr<Prefab::Service>(Prefab::Service::create(services)));

		// Add Serialization
		addCoreSystem(std::make_shared<Serialization::Service>());

		// Register components here
		services->get<ECS::Controller>()->registerAllComponents();
		// Register all systems here
		services->get<ECS::Controller>()->registerAllSystems();

		// Scenes
		addCoreSystem(std::make_shared<Scene::SceneManager>());

		// Camera System
		addCoreSystem(std::make_shared<sCameraController>());

		// Renderer
		addCoreSystem(std::make_shared<sRenderer>());


		//Editor only added when debug mode
#ifdef _DEBUG
		// !NOTE: IMGUI eats events
		auto editor = std::make_shared<Editor::Editor>(app_window->getNativeWindow());
		addLayerSystem(editor);
#endif


#ifdef _DEBUG
		PN_CORE_INFO("Testing Lua on Debug");
#else
		PN_CORE_INFO("Testing Lua on Release");
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

		if (window->isMinimized()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue; // Go back to start of while loop
		}

		//Drain all events in queue
		drainEventQueue();

		//Update delta time
		auto now = std::chrono::steady_clock::now();
		float real_dt = std::chrono::duration<float>(now - last_time).count();
		last_time = now;

		// Store Unscaled Time (Always ticking even when paused)
		timing.unscaled_dt = real_dt;

		auto fps = static_cast<int>(1.f / timing.unscaled_dt);
#ifdef PN_PLATFORM_WINDOWS
		static float avgFps = 0.f;
		static float timeSinceLastUpdate = 0.0f;

		avgFps = avgFps * 0.95f + fps * 0.05f;
		timeSinceLastUpdate += timing.unscaled_dt;

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
			timing.dt = real_dt;
			services->get<Audio::Audio>()->resumeAll();
		}
#else
		timing.dt = real_dt;
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
	}
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