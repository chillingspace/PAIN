#pragma once

#ifdef PN_PLATFORM_WINDOWS

	extern PAIN::Application* PAIN::CreateApplication();

	int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

		//// Enable run-time memory check for debug builds.
		#if defined(DEBUG) | defined(_DEBUG)
				_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		#endif

		PAIN::Log::Init();
		PN_CORE_WARN("Initialized Log!");
		int a = 5;
		PN_INFO("Hello! Var={}", a);

		auto game = PAIN::CreateApplication();
		game->Run();
		delete game;
	}

#endif