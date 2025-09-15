#include "pch.h"
#include "audio.h"

#include "FMOD/FMODAudio.h"

namespace PAIN {
	namespace Audio {

		Controller::Controller() {
			audio_system = std::make_shared<PNSystem>();

			//Test audio
			auto sound = audio_system->loadSound("../Game/assets/Test_Music.wav");
			audio_system->play(sound);
		}

		void Controller::onUpdate() {
			audio_system->update();
		}

		void Controller::onEvent([[maybe_unused]]Event::Event& e) {
			
		}
	}
}