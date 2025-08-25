#pragma once


#ifdef PN_PLATFORM_WINDOWS

	extern PAIN::Application* PAIN::CreateApplication();

	int main(int argc, char** argv) {
		printf("Starting PAINEngine\n");
		auto game = PAIN::CreateApplication();
		game->Run();
		delete game;
	}

#endif