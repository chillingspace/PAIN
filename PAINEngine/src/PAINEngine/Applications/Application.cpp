#include "pch.h"
#include "Application.h"

#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Events/Event.h"
#include "CoreSystems/Renderer/RendererLayer.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Audio/AudioManager.h"
#include "CoreSystems/Scene/Scene.h"

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

#include "Systems/Physics/sysPhysics.h"


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
#ifdef PN_PLATFORM_WINDOWS
		glfwGetFramebufferSize((GLFWwindow*)window->getNativeWindow(), &winWidth, &winHeight);
		glViewport(0, 0, winWidth, winHeight);
#else
		ANativeWindow* nativeWindow = (ANativeWindow*)window->getNativeWindow();
		winWidth = ANativeWindow_getWidth(nativeWindow);
		winHeight = ANativeWindow_getHeight(nativeWindow);
		glViewport(0, 0, winWidth, winHeight);
#endif

		// Create and add the AudioManager to the core systems
		//Create path service
		services->set<Path::Path>(std::shared_ptr<Path::Path>(Path::Path::create(app)));
		services->get<Path::Path>()->logVirtualPaths();

		//Create and add the AudioManager to the core systems
		auto app_audio = std::shared_ptr<Audio::Audio>(Audio::Audio::create(app));
		addCoreSystem(app_audio);

		//Audio testing.
		auto asset_path = services->get<Path::Path>()->resolvePath("game_assets://Audio/Music/Boss_Music.wav");
		PN_CORE_INFO(asset_path);
		app_audio->loadSound(asset_path, true, false, false);
		//app_audio->play(asset_path);

		//Push other core systems into the stack
		addCoreSystem(std::make_shared<ECS::Controller>());
		addCoreSystem(std::make_shared<MetaData::Service>());

		// Add Serialization
#ifdef PN_PLATFORM_WINDOWS
		//addCoreSystem(std::make_shared<Serialization::Service>());
		// Add systems here

		// Physics system not cross platform yet
		services->get<ECS::Controller>()->registerSystem<Physics::System>(false);
#endif

		// Register components here
		services->get<ECS::Controller>()->registerComponent<Metadata>();
		services->get<ECS::Controller>()->registerComponent<Transform>();
		services->get<ECS::Controller>()->registerComponent<MeshRenderer>();

		// Windows only have paths, andriods have to use AASettmanager
#ifdef PN_PLATFORM_WINDOWS
		addCoreSystem(std::make_shared<Path::Service>());
		services->get<Path::Service>()->init("assets/Config.json");
		addCoreSystem(std::make_shared<Assets::Service>());
		addCoreSystem(std::make_shared<Compiler::Service>());
#endif
		// Scenes
		addCoreSystem(std::make_shared<Scene>());

		// Renderer
		addCoreSystem(std::make_shared<RendererLayer>());


		//Editor only added when debug mode
#ifdef _DEBUG
		// !NOTE: IMGUI eats events
		auto editor = std::make_shared<Editor::Editor>(app_window->getNativeWindow());
		addLayerSystem(editor);
#endif

		//Mark engine as ready
		b_app_running = true;
	}

	void Application::Run() {

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

			fps = static_cast<int>(1.f / timing.dt);
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


				//Update all core systems
				for (auto& core : core_stack) core.lock()->onFixedUpdate(timing);

				//Update all layered systems
				for (auto& layer : layer_stack) layer.lock()->onFixedUpdate(timing);



				accumulator -= timing.fixed_dt;
				++steps;
			}

			//Update timing variables
			timing.steps_this_frame = steps;
			timing.alpha = static_cast<float>(accumulator / timing.fixed_dt);

			//Update all core systems
			for (auto& core : core_stack) core.lock()->onUpdate(timing);

			//Update all layered systems
			for (auto& layer : layer_stack) layer.lock()->onUpdate(timing);

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
			(*it).lock()->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}

		//Check if handled
		if (handled) return;

		//Dispatch to layer bottom up
		for (auto it = layer_stack.begin(); it != layer_stack.end(); ++it) {

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

			//Dispatch event down layers
			(*it).lock()->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}

		//Check if handled
		if (handled) return;

		//Dispatch to core top down
		for (auto it = core_stack.rbegin(); it != core_stack.rend(); ++it) {

			//Dispatch event down layers
			(*it).lock()->onEvent(e);
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