#include "PAINEngine.h" // Your main engine include
#include "Logging/Log.h"  // Your logging system

#ifdef _WIN32
#include <Windows.h>
#endif

// Your Game class, now in main.cpp
class Game : public PAIN::Application
{
public:
    Game() 
    {
        // Game-specific initialization can go here
    }
    ~Game() 
    {
        // Game-specific cleanup
    }
};

// The CreateApplication function that the entry point needs
PAIN::Application* PAIN::CreateApplication() {
    return new Game();
}

// The main entry point for the application
int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) 
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    PAIN::Log::Init();
    PN_CORE_WARN("Initialized Log!");

    auto game = PAIN::CreateApplication();
    game->Run();
    delete game;

    return 0;
}