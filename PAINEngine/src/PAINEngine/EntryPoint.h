#pragma once


#ifdef PN_PLATFORM_WINDOWS

	extern PAIN::Application* PAIN::CreateApplication();

	int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {

		PAIN::Log::Init();
		PN_CORE_WARN("Initialized Log!");
		int a = 5;
		PN_INFO("Hello! Var={}", a);

		//printf("Starting PAINEngine\n");
		auto game = PAIN::CreateApplication();
		game->Run();
		delete game;
	}

#endif