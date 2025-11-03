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
		int* leak = new int[1];

		auto game = PAIN::CreateApplication();
		game->Init();
		game->Run();
		delete game;
	}

#endif