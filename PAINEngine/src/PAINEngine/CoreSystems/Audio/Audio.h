#pragma once

#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "Applications/AppSystem.h"

namespace PAIN {
	namespace Audio {

		class Audio {
		private:
		public:
			Audio() = default;
			~Audio() = default;
		};

		class ChannelGroup {
		private:
		public:
			ChannelGroup() = default;
			~ChannelGroup() = default;
		};

		class Channel {
		private:
		public:
			Channel() = default;
			~Channel() = default;
		};

		class System {
		private:
		public:
			System() = default;
			~System() = default;

			//Update system
			virtual void update() = 0;

			virtual std::shared_ptr<Audio> loadSound(const std::string& path) = 0;

			virtual std::shared_ptr<Channel> play(const std::shared_ptr<Audio>& sound, bool paused = false, ChannelGroup* group = nullptr) = 0;

			virtual ChannelGroup createGroup(const char* name) = 0;
		};

		//Audio controller
		class Controller : public AppSystem {
		private:
			std::shared_ptr<System> audio_system;
		public:
			Controller();

			//Optional virtual functions
			void onUpdate() override;

			//Event handler for app layer
			void onEvent(Event::Event& e) override;
		};
	}
}

#endif
