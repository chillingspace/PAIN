#include "pch.h"
#include "Application.h"

#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Events/Event.h"
#include "CoreSystems/Renderer/RendererLayer.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Audio/AudioManager.h"

#include "LayeredSystems/LevelEditor/Editor.h"

#include "ECS/Controller.h"

#ifdef PN_PLATFORM_WINDOWS
#include "Scene/Scene.h"
#endif

// Assets
#include "CoreSystems/Assets/sPath.h"
#include "CoreSystems/Assets/sLoader.h"
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Assets/sAssetCompiler.h"


namespace PAIN {

	Application::Application() {
		//Create default services
		services = std::make_shared<Services>();
	}

	Application::~Application() {
		//Destroy top down
		for (auto it = layer_stack.rbegin(); it != layer_stack.rend(); ++it) {

			//On detach
			(*it)->onDetach();
		}
		layer_stack.clear();

		//Destroy to core top down
		for (auto it = core_stack.rbegin(); it != core_stack.rend(); ++it) {

			//On detach
			(*it)->onDetach();
		}
		core_stack.clear();
	}

	template<typename T>
	void Application::addCoreSystem(std::shared_ptr<T> core_system) {
		PN_CORE_INFO("jspoh addcore");
		core_system->services = services;
		PN_CORE_INFO("jspoh addcore after services");
		core_system->onAttach();
		PN_CORE_INFO("jspoh addcore after onAttach");
		core_stack.push_back(core_system);
		services->set<T>(core_system);
	}

	template<typename T>
	void Application::addLayerSystem(std::shared_ptr<T> layer_system) {
		layer_system->services = services;
		layer_system->onAttach();
		layer_stack.push_back(layer_system);
		services->set<T>(layer_system);
	}

	void Application::Init(void* app) {

		//Initialize logger
		PAIN::Log::Init();
		PN_CORE_INFO("Initialized Log!");

		auto app_window = std::shared_ptr<Window::Window>(Window::Window::create(app));
		app_window->registerCallbacks(this);
		addCoreSystem(app_window);

		// Create and add the AudioManager to the core systems
		auto app_audio = std::shared_ptr<Audio::Audio>(Audio::Audio::create(app));
		addCoreSystem(app_audio);

		//Audio testing.
#ifdef PN_PLATFORM_ANDROID

		//Android specific paths, will need to abstract this out
		app_audio->loadSound("file:///android_asset/audio/Music/Boss_Music.wav", true, false, false);
		app_audio->play("file:///android_asset/audio/Music/Boss_Music.wav");
#else
		app_audio->loadSound("assets/audio/Music/Boss_Music.wav", true, false, false);
		//app_audio->play("assets/audio/Music/Boss_Music.wav");
#endif


//Push other core systems into the stack
//addCoreSystem(window_app);
		addCoreSystem(std::make_shared<ECS::Controller>());

		// Windows only have paths, andriods have to use AASettmanager
#ifdef PN_PLATFORM_WINDOWS
		addCoreSystem(std::make_shared<Path::Service>());
		services->get<Path::Service>()->init("assets/Config.json");
		addCoreSystem(std::make_shared<Assets::Service>());
		addCoreSystem(std::make_shared<Loader::Service>());
		addCoreSystem(std::make_shared<Compiler::Service>());
#endif

		PN_CORE_INFO("jspoh1");
		auto renderer = std::make_shared<RendererLayer>();
		PN_CORE_INFO("jspoh2");
		addCoreSystem(renderer);
		PN_CORE_INFO("jspoh3");

		//Editor only added when debug mode
#ifdef _DEBUG
		// !NOTE: IMGUI eats events
		addLayerSystem(std::make_shared<Editor::Editor>(app_window->getNativeWindow()));
#endif

		//Mark engine as ready
		b_app_running = true;
	}

	void Application::Run() {
#ifdef PN_PLATFORM_WINDOWS
		Scene scene;
		scene.Init();
#endif

		//Set last time
		last_time = std::chrono::steady_clock::now();

		//Application loop
		while (b_app_running) {

			//static auto last_time = std::chrono::high_resolution_clock::now();
			//auto current_time = std::chrono::high_resolution_clock::now();
			//float dt = std::chrono::duration<float, std::chrono::seconds::period>(current_time - last_time).count();
			//last_time = current_time;

			//Poll events
			services->get<Window::Window>()->pollEvents();

			//Drain all events in queue
			drainEventQueue();

			//Update delta time
			auto now = std::chrono::steady_clock::now();
			timing.dt = std::chrono::duration<float>(now - last_time).count();
			last_time = now;

			//Accumulate for fixed updates
			accumulator += timing.dt;

			//Skip all other systems when window is not active
			if (!services->get<Window::Window>()->getActive()) {
				services->get<Window::Window>()->swapBuffers();
				continue;
			}


			//Update fixed delta
			int steps = 0;
			while (accumulator >= timing.fixed_dt && steps < MAX_STEPS) {
#ifdef PN_PLATFORM_WINDOWS
				scene.OnUpdate();
#endif
				//Update all core systems
				for (auto& core : core_stack) core->onFixedUpdate(timing);

				//Update all layered systems
				for (auto& layer : layer_stack) layer->onFixedUpdate(timing);



				accumulator -= timing.fixed_dt;
				++steps;
			}

			//Update timing variables
			timing.steps_this_frame = steps;
			timing.alpha = static_cast<float>(accumulator / timing.fixed_dt);

			//Update all core systems
			for (auto& core : core_stack) core->onUpdate(timing);

			//Update all layered systems
			for (auto& layer : layer_stack) layer->onUpdate(timing);

			//Swap buffer
			services->get<Window::Window>()->swapBuffers();

			//PN_CORE_INFO("LOOP");
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

			//Dispatch event down layers
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}

		//Check if handled
		if (handled) return;

		//Dispatch to layer bottom up
		for (auto it = layer_stack.begin(); it != layer_stack.end(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}
	}

	void Application::dispatchEventsReversed(Event::Event& e) {
		//Boolean flag for propogation
		bool handled = false;

		//Dispatch to layer top down
		for (auto it = layer_stack.rbegin(); it != layer_stack.rend(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}

		//Check if handled
		if (handled) return;

		//Dispatch to core top down
		for (auto it = core_stack.rbegin(); it != core_stack.rend(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
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