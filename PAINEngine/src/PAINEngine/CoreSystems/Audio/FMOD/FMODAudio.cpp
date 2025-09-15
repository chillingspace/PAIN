#include "pch.h"
#include "FMODAudio.h"

namespace PAIN {
	namespace Audio {

		PNSystem::PNSystem() {
			FMOD_CHECK(FMOD::System_Create(&fmod_system));
			FMOD_CHECK(fmod_system->init(512, FMOD_INIT_NORMAL, nullptr));
			FMOD_CHECK(fmod_system->getMasterChannelGroup(&master));
		}

		PNSystem::~PNSystem() {
			// Release cached sounds before system shutdown
			cache.clear();
			if (fmod_system) { fmod_system->close(); fmod_system->release(); fmod_system = nullptr; }
		}


        void PNSystem::update() { 
            if (fmod_system) fmod_system->update(); 

            int channels = 0;
            fmod_system->getChannelsPlaying(&channels);
            PN_CORE_INFO("{}", channels);
        }

        std::shared_ptr<Audio> PNSystem::loadSound(const std::string& path) {
            if (auto it = cache.find(path); it != cache.end()) {
                if (auto s = it->second.lock()) return s;
            }
            FMOD::Sound* raw = nullptr;
            FMOD_CHECK(fmod_system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &raw));
            //unsigned int duration = 0;
            //raw->getLength(&duration, FMOD_TIMEUNIT_MS);
            //PN_CORE_INFO("{}", duration);
            auto shared = make_shared_sound(raw);
            auto out = std::make_shared<PNAudio>(shared, path);
            cache[path] = out;
            return out;
        }

        std::shared_ptr<Channel> PNSystem::play(const std::shared_ptr<Audio>& sound, bool paused, ChannelGroup* group) {
            FMOD::Channel* ch = nullptr;
            FMOD::ChannelGroup* grp = group ? static_cast<PNChannelGroup*>(group)->get() : master;
            std::shared_ptr<PNAudio> pn_sound = std::static_pointer_cast<PNAudio>(sound);

            if (!pn_sound) { 
                PN_CORE_WARN("INVALID SOUND PASSED IN. ABORT."); return nullptr;
            }
            else {
                FMOD_CHECK(fmod_system->playSound(pn_sound->get(), grp, paused, &ch));
            }

            return std::make_shared<PNChannel>(ch, pn_sound);
        }

        ChannelGroup PNSystem::createGroup(const char* name) {
            FMOD::ChannelGroup* grp = nullptr;
            FMOD_CHECK(fmod_system->createChannelGroup(name, &grp));
            FMOD_CHECK(master->addGroup(grp));
            return PNChannelGroup(grp);
        }
	}
}