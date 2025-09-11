#include "stdafx.h"
#include <PAINEngine.h>
#include <PAINEngine/Core/EntryPoint.h>

class Game : public PAIN::Application
{
	public: 
		Game() {

		}
		~Game() {

		}
};

PAIN::Application* PAIN::CreateApplication() {

	// Create a simple window
	return new Game();
}