#pragma once

// This header is the entry point for the application and should only be included by the client (Game.cpp)

#ifdef PN_PLATFORM_WINDOWS

	// Forward declare the function to create the application, which is defined in Game.cpp
	extern PAIN::Application* PAIN::CreateApplication();

	int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

		// Enable run-time memory check for debug builds.
		#if defined(DEBUG) | defined(_DEBUG)
				_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		#endif

		PAIN::Log::Init();
		PN_CORE_WARN("Initialized Log!");

		auto game = PAIN::CreateApplication();
		game->Run();
		delete game;
	}

#endif