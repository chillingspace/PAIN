#pragma once

#ifdef PN_PLATFORM_WINDOWS

	extern PAIN::Application* PAIN::CreateApplication();


	int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

		//// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

		{
		// Normal mode - initialize graphics and run game
		auto game = PAIN::CreateApplication();
		game->Init();
		game->Run();
		delete game;	
		}

		//new int[67]; // Intentional memory leak for testing

		// can't do this. intentional memory leak above was not caught
//#if defined(DEBUG) | defined(_DEBUG)
//    	_CrtDumpMemoryLeaks();   
//#endif
	}

#endif