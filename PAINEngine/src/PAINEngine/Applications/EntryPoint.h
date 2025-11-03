#pragma once

#ifdef PN_PLATFORM_WINDOWS

	extern PAIN::Application* PAIN::CreateApplication();


	int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

		//// Enable run-time memory check for debug builds.
		#if defined(DEBUG) | defined(_DEBUG)
				_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		#endif

		
		// For CI to run properly, to check for leaks and crashes
		if (std::getenv("CI_TEST") != nullptr) {
			std::thread([] {
				std::this_thread::sleep_for(std::chrono::seconds(10));
				std::exit(0);
				}).detach();
		}
		
		// Purposely added mem leak here to test in CI
		int* leak = new int[1];

		auto game = PAIN::CreateApplication();
		game->Init();
		game->Run();
		delete game;
	}

#endif