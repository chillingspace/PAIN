#include <PAINEngine.h>
#include <PAINEngine/EntryPoint.h>

class Game : public PAIN::Application
{
	public: 
		Game() {

		}
		~Game() {

		}
};

PAIN::Application* PAIN::CreateApplication() {
	return new Game();
}