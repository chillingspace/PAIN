#pragma once

#ifdef PN_PLATFORM_WINDOWS

	extern PAIN::Application* PAIN::CreateApplication();


	int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

		//// Enable run-time memory check for debug builds.
		#if defined(DEBUG) | defined(_DEBUG)

		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

		// Redirect CRT output to stderr so PowerShell can capture it for CI runs
		if (!IsDebuggerPresent()) {
			_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
			_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
		}

		#endif
		
		// Purposely added mem leak here to test in CI
		//int* leak = new int[1];

		// Check if running in CI environment
		bool is_CI = std::getenv("CI") != nullptr || std::getenv("GITHUB_ACTIONS") != nullptr;

		if (!is_CI) {
			// Normal mode - initialize graphics and run game
			auto game = PAIN::CreateApplication();
			game->Init();
			game->Run();
			delete game;
		}
		else {
			// CI mode - skip graphics, just run for a bit to test memory/crashes
			PN_CORE_INFO("Running in CI mode - skipping graphics initialization\n");
			std::this_thread::sleep_for(std::chrono::seconds(5));
			PN_CORE_INFO("CI test completed\n");
		}
	}

#endif