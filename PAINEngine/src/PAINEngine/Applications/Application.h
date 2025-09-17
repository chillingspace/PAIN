#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "AppSystem.h"
#include "PAINEngine/CoreSystems/Events/Event.h"
#include "PAINEngine/Audio/AudioManager.h"
#include "PAINEngine/CoreSystems/Windows/Window.h"

#include <memory>
#include <vector>
#include <queue>

#if defined(PLATFORM_ANDROID)
struct AAssetManager; // Forward declaration for Android Asset Manager
#endif

namespace PAIN {

    class Application
    {
    private:
        static Application* s_Instance;
        std::vector<std::shared_ptr<AppSystem>> layer_stack;
        std::vector<std::shared_ptr<AppSystem>> core_stack;
        bool b_app_running = true;
        std::queue<std::shared_ptr<Event::Event>> event_queue;
        std::shared_ptr<AudioManager> m_AudioManager;
        std::unique_ptr<Window::Window> m_Window;

        void dispatchEventsForward(Event::Event& e);
        void dispatchEventsReversed(Event::Event& e);

    public:
        Application();
        virtual ~Application();
        
        void InitializeDesktop();
#if defined(PLATFORM_ANDROID)
        void InitializeAndroid(AAssetManager* assetManager);
#endif

        void Run();
        void Update();
        void Shutdown();

        void addCoreSystem(std::shared_ptr<AppSystem> core_system);
        void addLayerSystem(std::shared_ptr<AppSystem> layer_system);

        void dispatchEvent(Event::Event& e);
        void pushEventQueue(std::shared_ptr<Event::Event> e);
        void drainEventQueue();

        inline static Application& Get() { return *s_Instance; }
        inline AudioManager& GetAudioManager() { return *m_AudioManager; }
    };

    Application* CreateApplication();
}

#endif