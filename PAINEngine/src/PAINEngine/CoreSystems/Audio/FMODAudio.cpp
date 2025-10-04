#include "pch.h"
#include "FMODAudio.h"

#ifdef PN_PLATFORM_ANDROID
//Android FMOD header
#include <fmod_android.h>
#endif

#include <fmod.hpp>
#include <fmod_common.h>

namespace PAIN {
	namespace Audio {

#undef min
#undef max

		static inline float db2lin(float db) { return std::pow(10.0f, db / 20.0f); }
		static inline float clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }

		Audio* Audio::create(void* app) {
			return new FmodAudio(app);
		}

		struct FmodAudio::Impl {
			// FMOD core
			FMOD::System* sys = nullptr;
			bool initialized = false;

			// assets
			struct SoundInfo {
				FMOD::Sound* sound = nullptr;
				bool is3D = true;
				bool looping = false;
				bool stream = false;
				float minDistance = 1.0f;
				float maxDistance = 50.0f;
			};
			std::unordered_map<std::string, SoundInfo> sounds;
			std::unordered_map<std::string, std::vector<std::string>> playlists;

			// channels
			int nextId = 0;
			struct Chan { FMOD::Channel* ch = nullptr; };
			std::unordered_map<int, Chan> channels;

			// groups
			struct Group {
				FMOD::ChannelGroup* cg = nullptr;
				float currentDb = 0.0f;
				float targetDb = 0.0f;
				float velDbPerSec = 0.0f; // simple 1st-order fade
			};
			std::unordered_map<std::string, Group> groups; // "master","music","sfx","ui"

			// RNG for playlists
			std::mt19937 rng{ std::random_device{}() };

			// conversions
			static FMOD_VECTOR toF(const glm::vec3& v) { return FMOD_VECTOR{ v.x, v.y, v.z }; }

			AudioResult ensureGroup(const char* name) {
				if (groups.count(name)) return AudioResult::Ok;
				Group g{};
				if (!sys) return AudioResult::NotInitialized;
				FMOD_RESULT r = sys->createChannelGroup(name, &g.cg);
				if (r != FMOD_OK) return AudioResult::BackendError;

				FMOD::ChannelGroup* master = nullptr;
				sys->getMasterChannelGroup(&master);
				if (master) master->addGroup(g.cg);

				groups.emplace(name, g);
				return AudioResult::Ok;
			}

			void tickFades(double dt) {
				for (auto& [name, g] : groups) {
					if (!g.cg) continue;
					if (std::abs(g.currentDb - g.targetDb) < 0.05f) {
						g.currentDb = g.targetDb;
					}
					else if (g.velDbPerSec != 0.0f) {
						g.currentDb += g.velDbPerSec * static_cast<float>(dt);
						if ((g.velDbPerSec > 0 && g.currentDb > g.targetDb) ||
							(g.velDbPerSec < 0 && g.currentDb < g.targetDb))
							g.currentDb = g.targetDb;
					}
					g.cg->setVolume(db2lin(g.currentDb));
				}
			}

			// Determines which channel group a sound belongs to based on its path
			FMOD::ChannelGroup* pickGroup(std::string_view path) {
				std::string path_lower(path);
				std::transform(path_lower.begin(), path_lower.end(), path_lower.begin(),
					[](unsigned char c) { return std::tolower(c); });

				if (path_lower.find("music") != std::string::npos) {
					return groups["music"].cg;
				}
				if (path_lower.find("sfx") != std::string::npos) {
					return groups["sfx"].cg;
				}
				if (path_lower.find("ui") != std::string::npos) {
					return groups["ui"].cg;
				}

				return groups["master"].cg; // Fallback to master
			}
		};

		FmodAudio::FmodAudio(void* app) : impl_(std::make_unique<Impl>()) {
#ifdef PN_PLATFORM_ANDROID
			m_App = static_cast<android_app*>(app);
#endif
		}
		FmodAudio::~FmodAudio() { shutdown(); }

#ifdef PN_PLATFORM_ANDROID
		void FmodAudio::fmodJNIAttach() {
			JavaVM* vm = m_App->activity->vm;
			JNIEnv* env = nullptr;

			jint result = vm->AttachCurrentThread(&env, nullptr);
			if (result != JNI_OK) {
				PN_CORE_ERROR("Failed to attach to JVM: %d", result);
				return;
			}
			
			try {
				FMOD_Android_JNI_Init(vm, m_App->activity->clazz);
			}
			catch (...) {
				PN_CORE_ERROR("FMOD JNI Init failed - continuing without JNI features");
			}
		}

		void FmodAudio::fmodJNIDetach() {
			try {
				FMOD_Android_JNI_Close();
			}
			catch (...) {}

			if (m_App && m_App->activity && m_App->activity->vm) {
				m_App->activity->vm->DetachCurrentThread();
			}
		}
#endif

		static AudioResult toResult(FMOD_RESULT r) {
			return r == FMOD_OK ? AudioResult::Ok : AudioResult::BackendError;
		}

		AudioResult FmodAudio::init() {

#ifdef PN_PLATFORM_ANDROID
			fmodJNIAttach();
#endif

			if (impl_->initialized) return AudioResult::AlreadyInitialized;

			FMOD_RESULT r = FMOD::System_Create(&impl_->sys);
			if (r != FMOD_OK) return AudioResult::BackendError;

			r = impl_->sys->init(1024, FMOD_INIT_3D_RIGHTHANDED, nullptr);
			if (r != FMOD_OK) return AudioResult::BackendError;

			impl_->sys->set3DSettings(1.0f, 1.0f, 1.0f);

			// Make default groups
			impl_->ensureGroup("master");
			impl_->ensureGroup("music");
			impl_->ensureGroup("sfx");
			impl_->ensureGroup("ui");

			impl_->initialized = true;
			return AudioResult::Ok;
		}

		void FmodAudio::shutdown() {
			if (!impl_->initialized) return;

			stopAll();

			for (auto& kv : impl_->sounds)
				if (kv.second.sound) kv.second.sound->release();
			impl_->sounds.clear();

			for (auto& kv : impl_->groups)
				if (kv.second.cg) kv.second.cg->release();
			impl_->groups.clear();

			if (impl_->sys) {
				impl_->sys->close();
				impl_->sys->release();
				impl_->sys = nullptr;
			}
			impl_->initialized = false;

#ifdef PN_PLATFORM_ANDROID
			fmodJNIDetach();
#endif
		}

		void FmodAudio::onUpdate(AppTiming timing) {
			if (!impl_->initialized) return;
			impl_->tickFades(timing.dt);
			impl_->sys->update();

			std::vector<int> toErase;
			for (auto& [id, c] : impl_->channels) {
				bool playing = false;
				if (c.ch && c.ch->isPlaying(&playing) == FMOD_OK && !playing)
					toErase.push_back(id);
				if (!c.ch) toErase.push_back(id);
			}
			for (int id : toErase) impl_->channels.erase(id);
		}

		AudioResult FmodAudio::loadSound(std::string_view path, bool is3D, bool looping, bool stream, float minDistance, float maxDistance) {
			if (!impl_->initialized) return AudioResult::NotInitialized;
			if (path.empty()) return AudioResult::InvalidArg;
			if (impl_->sounds.count(std::string(path))) return AudioResult::Ok;

			FMOD_MODE mode = FMOD_DEFAULT;
			if (is3D)   mode |= FMOD_3D; else mode |= FMOD_2D;
			if (looping) mode |= FMOD_LOOP_NORMAL; else mode |= FMOD_LOOP_OFF;
			if (stream)  mode |= FMOD_CREATESTREAM;

			FMOD::Sound* s = nullptr;
			FMOD_RESULT r = impl_->sys->createSound(std::string(path).c_str(), mode, nullptr, &s);
			if (r != FMOD_OK) return AudioResult::NotFound;

			if (is3D) {
				s->set3DMinMaxDistance(minDistance, maxDistance);
			}

			impl_->sounds.emplace(std::string(path), Impl::SoundInfo{ s, is3D, looping, stream, minDistance, maxDistance });
			return AudioResult::Ok;
		}

		AudioResult FmodAudio::loadPlaylist(const PlaylistDesc& desc) {
			if (desc.name.empty() || desc.paths.empty()) return AudioResult::InvalidArg;
			impl_->playlists[desc.name] = desc.paths;
			return AudioResult::Ok;
		}

		std::optional<AudioChannelId> FmodAudio::play(std::string_view soundPath, const glm::vec3& pos, float volumeDb) {
			if (!impl_->initialized) return std::nullopt;

			auto it = impl_->sounds.find(std::string(soundPath));
			if (it == impl_->sounds.end()) {
				if (loadSound(soundPath, true, false, false, 1.0f, 50.0f) != AudioResult::Ok) {
					return std::nullopt;
				}
				it = impl_->sounds.find(std::string(soundPath));
			}

			auto& si = it->second;

			si.sound->setMode((si.is3D ? FMOD_3D : FMOD_2D) | (si.looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | (si.stream ? FMOD_CREATESTREAM : FMOD_DEFAULT));
			if (si.is3D) {
				si.sound->set3DMinMaxDistance(si.minDistance, si.maxDistance);
			}

			FMOD::Channel* ch = nullptr;
			FMOD::ChannelGroup* cg = impl_->pickGroup(soundPath);

			FMOD_RESULT r = impl_->sys->playSound(si.sound, cg, true, &ch);
			if (r != FMOD_OK || !ch) return std::nullopt;

			if (si.is3D) {
				auto fpos = Impl::toF(pos);
				FMOD_VECTOR vel{ 0,0,0 };
				ch->set3DAttributes(&fpos, &vel);
			}
			ch->setVolume(db2lin(volumeDb));
			ch->setPaused(false);

			int id = impl_->nextId++;
			impl_->channels.emplace(id, Impl::Chan{ ch });
			return AudioChannelId{ id };
		}

		std::optional<AudioChannelId> FmodAudio::playRandom(std::string_view playlistName, const glm::vec3& pos, float volumeDb) {
			auto it = impl_->playlists.find(std::string(playlistName));
			if (it == impl_->playlists.end() || it->second.empty()) return std::nullopt;
			std::uniform_int_distribution<size_t> dist(0, it->second.size() - 1);
			const std::string& pick = it->second[dist(impl_->rng)];
			
			// Logging footstep check
			//PN_CORE_INFO("Playing footstep: {}", pick);

			return play(pick, pos, volumeDb);
		}

		AudioResult FmodAudio::stop(AudioChannelId chId) {
			if (!isValid(chId)) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			it->second.ch->stop();
			impl_->channels.erase(it);
			return AudioResult::Ok;
		}

		void FmodAudio::stopAll() {
			for (auto& [n, g] : impl_->groups)
				if (g.cg) g.cg->stop();
			impl_->channels.clear();
		}

		AudioResult FmodAudio::setVolumeDb(AudioChannelId chId, float db) {
			if (!isValid(chId)) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			return toResult(it->second.ch->setVolume(db2lin(db)));
		}

		AudioResult FmodAudio::setVolumeLinear(AudioChannelId chId, float v) {
			if (!isValid(chId)) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			return toResult(it->second.ch->setVolume(clamp01(v)));
		}

		AudioResult FmodAudio::setPosition(AudioChannelId chId, const glm::vec3& pos) {
			if (!isValid(chId)) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			auto fpos = Impl::toF(pos);
			FMOD_VECTOR vel{ 0,0,0 };
			return toResult(it->second.ch->set3DAttributes(&fpos, &vel));
		}

		void FmodAudio::setListener(const glm::vec3& pos, const glm::vec3& vel, const glm::vec3& fwd, const glm::vec3& up) {
			if (!impl_->initialized) return;
			auto fpos = Impl::toF(pos);
			auto fvel = Impl::toF(vel);
			auto ffwd = Impl::toF(fwd);
			auto fup = Impl::toF(up);
			impl_->sys->set3DListenerAttributes(0, &fpos, &fvel, &ffwd, &fup);
		}

		AudioResult FmodAudio::setGroupVolumeDb(const char* group, float db) {
			if (auto r = impl_->ensureGroup(group); r != AudioResult::Ok) return r;
			auto& g = impl_->groups[group];
			g.currentDb = db;
			g.targetDb = db;
			g.velDbPerSec = 0.0f;
			if (g.cg) g.cg->setVolume(db2lin(db));
			return AudioResult::Ok;
		}

		AudioResult FmodAudio::fadeGroupToDb(const char* group, float targetDb, float seconds) {
			if (auto r = impl_->ensureGroup(group); r != AudioResult::Ok) return r;
			auto& g = impl_->groups[group];
			g.targetDb = targetDb;
			float dur = std::max(0.001f, (float)seconds);
			g.velDbPerSec = (g.targetDb - g.currentDb) / dur;
			return AudioResult::Ok;
		}

		void FmodAudio::onAppPause() {
			if (!impl_->initialized) return;
			impl_->sys->mixerSuspend();
		}
		void FmodAudio::onAppResume() {
			if (!impl_->initialized) return;
			impl_->sys->mixerResume();
		}
	}
}