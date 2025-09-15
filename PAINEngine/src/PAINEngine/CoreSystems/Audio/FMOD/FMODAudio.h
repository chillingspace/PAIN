#pragma once

#include "../Audio.h"

//FMOD Checker
#define FMOD_CHECK(x) do { FMOD_RESULT _r = (x); if (_r != FMOD_OK) throw std::runtime_error("FMOD Error"); } while(0)


namespace PAIN {
	namespace Audio {

		// Shared deleter for sounds
		inline std::shared_ptr<FMOD::Sound> make_shared_sound(FMOD::Sound* raw) {
			return std::shared_ptr<FMOD::Sound>(raw, [](FMOD::Sound* s) { if (s) s->release(); });
		}

		class PNAudio : public Audio {
		private:
			std::shared_ptr<FMOD::Sound> sound;
			std::string file_path;
		public:

			PNAudio(std::shared_ptr<FMOD::Sound> sound, const std::string& file_path) : sound{ sound }, file_path{ file_path }{}
			~PNAudio() = default;

			FMOD::Sound* get() const { return sound.get(); }

			const std::string& path() const { return file_path; }
		};

		class PNChannelGroup : public ChannelGroup {
		private:
			FMOD::ChannelGroup* group = nullptr;
		public:

			PNChannelGroup(FMOD::ChannelGroup* group) : group{ group }{}
			~PNChannelGroup() = default;

			FMOD::ChannelGroup* get() const { return group; }
		};

		class PNChannel : public Channel {
		private:
			FMOD::Channel* channel = nullptr;

			//Pointer to sound its currently playing
			std::shared_ptr<PNAudio> sound;
		public:
			PNChannel(FMOD::Channel* channel, std::shared_ptr<PNAudio> sound) : channel{ channel }, sound{ sound } {}
			~PNChannel() = default;

			FMOD::Channel* get() const { return channel; }
		};

		class PNSystem : public System {
		private:

			//Fmod System
			FMOD::System* fmod_system = nullptr;

			//Master channel group
			FMOD::ChannelGroup* master = nullptr; 

			//Audio cache
			std::unordered_map<std::string, std::weak_ptr<PNAudio>> cache;
		public:

			PNSystem();
			~PNSystem();

			//Update system
			void update() override;

			std::shared_ptr<Audio> loadSound(const std::string& path) override;

			std::shared_ptr<Channel> play(const std::shared_ptr<Audio>& sound, bool paused = false, ChannelGroup* group = nullptr) override;

			ChannelGroup createGroup(const char* name) override;
		};
	}
}