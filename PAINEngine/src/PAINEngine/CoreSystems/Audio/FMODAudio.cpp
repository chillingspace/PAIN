#include "pch.h"
#include "FMODAudio.h"

#ifdef PN_PLATFORM_ANDROID
//Android FMOD header
#include <fmod_android.h>
#endif

#include <fmod.hpp>
#include <fmod_common.h>

#include "CoreSystems/Assets/sAssets.h"

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
			struct FmodSound : public PAIN::Audio::Sound {
			public:
				FmodSound(const std::string& path, FMOD::Sound* fmodPtr)
					: path(path), sound(fmodPtr) {
				}

				~FmodSound() override {
					release();
				}
				std::string getPath() const override { return path; }
				FMOD::Sound* getFmodPtr() const { return sound; }
				void release() override {
					if (sound) {
						sound->release();
						sound = nullptr;
					}
				}

			private:
				std::string path;
				FMOD::Sound* sound;
			};
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
				for (auto& pair : groups) {
					auto& g = pair.second;
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

				//Fing group
				for (auto const& group : groups) {
					if (path_lower.find(group.first) != std::string::npos) {
						return group.second.cg;
					}
				}

				//Fall back to master
				return groups["master"].cg;
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

		struct UserData {
			std::shared_ptr<Path::Path> path_service;
		};

		static UserData* g_fmodUserData = nullptr;

		static FMOD_RESULT F_CALL myOpenCallback(const char* name, unsigned int* filesize, void** handle, void* userData) {
			//Get global user data
			auto data = g_fmodUserData;
			auto stream = data->path_service->createFileStream(name, Path::FileMode::Read);
			if (!stream || !stream->good()) return FMOD_ERR_FILE_NOTFOUND;
			*filesize = stream->size();
			*handle = stream.release();
			return FMOD_OK;
		}

		static FMOD_RESULT F_CALL myCloseCallback(void* handle, void* userData) {
			auto stream = static_cast<Path::IFileStream*>(handle);
			delete stream; // Or use smart pointer
			return FMOD_OK;
		}

		static FMOD_RESULT F_CALL myReadCallback(void* handle, void* buffer, unsigned int sizebytes, unsigned int* bytesread, void* userData) {
			auto stream = static_cast<Path::IFileStream*>(handle);
			*bytesread = (unsigned int)stream->read(buffer, sizebytes);
			if ((*bytesread > 0)) {
				return FMOD_OK;
			} 
			else {
				return FMOD_ERR_FILE_EOF;
			}
			return (*bytesread > 0) ? FMOD_OK : FMOD_ERR_FILE_EOF;
		}

		static FMOD_RESULT F_CALL mySeekCallback(void* handle, unsigned int pos, void* userData) {
			auto stream = static_cast<Path::IFileStream*>(handle);
			stream->seek(pos); // Add your own seek method
			return FMOD_OK;
		}

		AudioResult FmodAudio::init() {

#ifdef PN_PLATFORM_ANDROID
			fmodJNIAttach();
#endif

			//Init path service
			path_service = services->get<Path::Path>();

			if (impl_->initialized) return AudioResult::AlreadyInitialized;

			FMOD_RESULT r = FMOD::System_Create(&impl_->sys);
			if (r != FMOD_OK) return AudioResult::BackendError;

			r = impl_->sys->init(1024, FMOD_INIT_3D_RIGHTHANDED, nullptr);
			if (r != FMOD_OK) return AudioResult::BackendError;

			impl_->sys->set3DSettings(1.0f, 1.0f, 1.0f);

			// Get the REAL master group
			FMOD::ChannelGroup* masterGroup = nullptr;
			r = impl_->sys->getMasterChannelGroup(&masterGroup);
			if (r != FMOD_OK) return AudioResult::BackendError;

			// Store it in our map
			Impl::Group masterGroupWrapper;
			masterGroupWrapper.cg = masterGroup;
			impl_->groups.emplace("master", masterGroupWrapper);

			// Create other groups (which will now be correctly parented to the real master)
			for (auto group : group_names) {
				std::transform(group.begin(), group.end(), group.begin(),
					[](unsigned char c) { return std::tolower(c); });
				impl_->ensureGroup(group.c_str());
			}

			impl_->initialized = true;

			//Setup callbacks
			FMOD_FILE_OPEN_CALLBACK openCB = &myOpenCallback;
			FMOD_FILE_CLOSE_CALLBACK closeCB = &myCloseCallback;
			FMOD_FILE_READ_CALLBACK readCB = &myReadCallback;
			FMOD_FILE_SEEK_CALLBACK seekCB = &mySeekCallback;

			//Set user data
			g_fmodUserData = new UserData;
			g_fmodUserData->path_service = path_service;

			//Set up call backs
			impl_->sys->setFileSystem(
				openCB, closeCB,
				readCB, seekCB,
				nullptr, nullptr,
				2048);
			return AudioResult::Ok;
		}

		void FmodAudio::shutdown() {
			if (!impl_->initialized) return;

			stopAll();

			//Release all audio first
			auto asset_service = services->get<Assets::Manager>();
			auto sounds = asset_service->getAllAssetsOfType<Sound>(Assets::Type::Audio);

			//Release all sounds
			for (auto const& sound : sounds) {
				sound->release();
			}

			for (auto& kv : impl_->groups)
				if (kv.second.cg) kv.second.cg->release();
			impl_->groups.clear();

			if (impl_->sys) {
				impl_->sys->close();
				impl_->sys->release();
				impl_->sys = nullptr;
			}
			impl_->initialized = false;

			delete g_fmodUserData;
#ifdef PN_PLATFORM_ANDROID
			fmodJNIDetach();
#endif
		}

		void FmodAudio::onUpdate(AppTiming timing) {
			if (!impl_->initialized) return;
			impl_->tickFades(timing.dt);
			impl_->sys->update();

			std::vector<int> toErase;
			for (auto& pair : impl_->channels) {
				auto& c = pair.second;
				bool playing = false;
				if (c.ch && c.ch->isPlaying(&playing) == FMOD_OK && !playing)
					toErase.push_back(pair.first);
				if (!c.ch) toErase.push_back(pair.first);
			}
			for (int id : toErase) impl_->channels.erase(id);
		}

		std::shared_ptr<Sound> FmodAudio::createSound(std::string const& virtual_path) {
			if (!impl_->initialized) throw std::runtime_error("Initialization failed.");

			//Create sound options
			auto is_music = Assets::isMusic(path_service->resolvePath(virtual_path));
			FMOD_MODE mode = FMOD_DEFAULT;
			if (is_music) {
				mode |= FMOD_2D;
				mode |= FMOD_CREATESTREAM;
				mode |= FMOD_LOOP_NORMAL;
			}
			else {
				mode |= FMOD_3D;
				mode |= FMOD_LOOP_OFF;
			}

			FMOD::Sound* s = nullptr;
			FMOD_RESULT r = impl_->sys->createSound(virtual_path.c_str(), mode, nullptr, &s);
			if (r != FMOD_OK) throw std::runtime_error("Failed to create sound.");

			if (mode & FMOD_3D) {
				s->set3DMinMaxDistance(MIN_DISTANCE_3D, MAX_DISTANCE_3D);
			}
			auto sound = std::make_shared<FmodAudio::Impl::FmodSound>(virtual_path, s);
			sound->stream = is_music ? true : false;
			return sound;
		}

		AudioResult FmodAudio::loadPlaylist(const PlaylistDesc& desc) {
			if (desc.name.empty() || desc.paths.empty()) return AudioResult::InvalidArg;
			impl_->playlists[desc.name] = desc.paths;
			return AudioResult::Ok;
		}

		std::optional<AudioChannelId> FmodAudio::play(std::shared_ptr<Sound> sound, std::string const& group, float vol, float pitch, bool looping, bool is3D, const glm::vec3& pos, float min_dist, float max_dist) {
			if (!impl_->initialized) return std::nullopt;

			//Check if sound is valid
			if(!sound) return std::nullopt;

			//Cast fmod audio up
			std::shared_ptr<FmodAudio::Impl::FmodSound> fmod_sound = std::dynamic_pointer_cast<FmodAudio::Impl::FmodSound>(sound);

			//Check for valid cast
			if(!fmod_sound) return std::nullopt;

			//Set mode
			fmod_sound->getFmodPtr()->setMode((is3D ? FMOD_3D : FMOD_2D) | (looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | (fmod_sound->stream ? FMOD_CREATESTREAM : FMOD_DEFAULT));
			if (is3D) {
				fmod_sound->getFmodPtr()->set3DMinMaxDistance(min_dist, max_dist);
			}

			//Pick group &  channel
			FMOD::Channel* ch = nullptr;
			FMOD::ChannelGroup* cg;

			//Check if valid group has been provided
			auto g_it = impl_->groups.find(group);
			if (g_it != impl_->groups.end()) {
				cg = g_it->second.cg;
			}
			else {
				cg = impl_->pickGroup(fmod_sound->getPath());
			}

			//Play sound
			FMOD_RESULT r = impl_->sys->playSound(fmod_sound->getFmodPtr(), cg, true, &ch);
			if (r != FMOD_OK || !ch) return std::nullopt;

			//Set 3D attributes
			if (is3D) {
				auto fpos = Impl::toF(pos);
				FMOD_VECTOR vel{ 0,0,0 };
				ch->set3DAttributes(&fpos, &vel);
			}
			ch->setVolume(db2lin(vol));
			ch->setPitch(db2lin(pitch));
			ch->setPaused(false);

			//Get new channel
			int id = impl_->nextId++;
			impl_->channels.emplace(id, Impl::Chan{ ch });
			return AudioChannelId{ id };
		}

		std::optional<AudioChannelId> FmodAudio::playRandom(std::string_view playlistName, const glm::vec3& pos, float volumeDb) {
			//auto it = impl_->playlists.find(std::string(playlistName));
			//if (it == impl_->playlists.end() || it->second.empty()) return std::nullopt;
			//std::uniform_int_distribution<size_t> dist(0, it->second.size() - 1);
			//const std::string& pick = it->second[dist(impl_->rng)];

			//// Logging footstep check
			////PN_CORE_INFO("Playing footstep: {}", pick);

			//return play(pick, pos, volumeDb);
			return std::nullopt;
		}

		AudioResult FmodAudio::stop(AudioChannelId chId) {
			if (!chId.isValid()) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			it->second.ch->stop();
			impl_->channels.erase(it);
			return AudioResult::Ok;
		}

		bool FmodAudio::isChannelValid(AudioChannelId chId) {
			if (!chId.isValid()) return false;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return false;
			bool playing = false;
			FMOD_RESULT result = it->second.ch->isPlaying(&playing);
			return (result == FMOD_OK && playing);
		}

		void FmodAudio::stopAll() {
			for (auto& pair : impl_->groups) {
				auto& g = pair.second;
				if (g.cg) g.cg->stop();
			}
			impl_->channels.clear();
		}

		AudioResult FmodAudio::pauseChannel(AudioChannelId chId) {
			if (!chId.isValid()) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			return toResult(it->second.ch->setPaused(true));
		}

		AudioResult FmodAudio::resumeChannel(AudioChannelId chId) {
			if (!chId.isValid()) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			return toResult(it->second.ch->setPaused(false));
		}

		void FmodAudio::pauseAll()
		{


			if (!impl_->initialized) {
				PN_CORE_WARN("[FMOD] pauseAll skipped - not initialized");
				return;
			}
			// 1. ANDROID SPECIFIC: Suspend the mixer thread
			// This stops the CPU usage and releases audio hardware to the OS
#ifdef PN_PLATFORM_ANDROID
			// NOTE: We no longer suspend the mixer here because calling FMOD API
			// after mixerSuspend results in undefined behavior (and failures to resume).
			// mixerSuspend is now strictly handled by onAppPause/onAppResume.
#endif

			// 2. LOGIC: Pause the Master Group Only
			// This creates a "Global Pause" without modifying the Music/SFX flags.
			// When we unpause Master, Music/SFX will return to whatever state they were in before.
			auto it = impl_->groups.find("master");
			if (it != impl_->groups.end() && it->second.cg) {
				if (it->second.cg) {
					it->second.cg->setPaused(true);
					PN_CORE_INFO("[FMOD] pauseAll() called");
				}
				else {
					PN_CORE_ERROR("[FMOD] Master Group pointer is NULL!");
				}

			}
			else {
				PN_CORE_ERROR("[FMOD] Could not find 'master' group in map!");
			}

		}

		void FmodAudio::resumeAll()
		{


			if (!impl_->initialized) {
				PN_CORE_WARN("[FMOD] resumeAll skipped - not initialized");
				return;
			}
			// ANDROID SPECIFIC: Resume the mixer thread
#ifdef PN_PLATFORM_ANDROID
			// NOTE: We no longer resume the mixer here. See pauseAll() for details.
#endif
			
			// 2. LOGIC: Unpause Master Group
			auto it = impl_->groups.find("master");
			if (it != impl_->groups.end() && it->second.cg) {
				if (it->second.cg) {
					it->second.cg->setPaused(false);
					PN_CORE_INFO("[FMOD] resumeAll() called");
				}
				else {
					PN_CORE_ERROR("[FMOD] Master Group pointer is NULL!");
				}
			}
			else {
				PN_CORE_ERROR("[FMOD] Could not find 'master' group in map!");
			}
		}

		AudioResult FmodAudio::setMuteAll(bool mute) {
			if (!impl_->initialized) return AudioResult::NotInitialized;

			// GET the "master" group, which was stored during init
			auto it = impl_->groups.find("master");
			if (it == impl_->groups.end()) {
				return AudioResult::NotFound; // Should not happen if init() was successful
			}

			auto& masterGroup = it->second;
			if (!masterGroup.cg) return AudioResult::NotFound;

			// Use setMute, which is separate from setPaused
			PN_CORE_INFO("Setting Master Mute: {}", mute);
			return toResult(masterGroup.cg->setMute(mute));
		}

		AudioResult FmodAudio::setVolumeDb(AudioChannelId chId, float db) {
			if (!chId.isValid()) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			return toResult(it->second.ch->setVolume(db2lin(db)));
		}

		AudioResult FmodAudio::setVolumeLinear(AudioChannelId chId, float v) {
			if (!chId.isValid()) return AudioResult::InvalidArg;
			auto it = impl_->channels.find(chId.value);
			if (it == impl_->channels.end() || !it->second.ch) return AudioResult::NotFound;
			return toResult(it->second.ch->setVolume(clamp01(v)));
		}

		AudioResult FmodAudio::setPosition(AudioChannelId chId, const glm::vec3& pos) {
			if (!chId.isValid()) return AudioResult::InvalidArg;
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

		std::vector<std::string> FmodAudio::getAllGroups() {
			std::vector<std::string> temp;
			for (auto const& group : impl_->groups) {
				temp.push_back(group.first);
			}
			return temp;
		}

		float FmodAudio::getGroupVolumeDb(const char* group) {
			if (auto r = impl_->ensureGroup(group); r != AudioResult::Ok) return 0.0f;
			auto& g = impl_->groups[group];
			return g.currentDb;
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

		bool FmodAudio::checkGroupIsPlaying(const char* group) {
			bool is_playing = false;
			if (auto r = impl_->ensureGroup(group); r != AudioResult::Ok) return is_playing;
			auto& g = impl_->groups[group];
			if (g.cg) g.cg->isPlaying(&is_playing);
			return is_playing;
		}

		bool FmodAudio::checkGroupPaused(const char* group) {
			bool paused = false;
			if (auto r = impl_->ensureGroup(group); r != AudioResult::Ok) return paused;
			auto& g = impl_->groups[group];
			if (g.cg) g.cg->getPaused(&paused);
			return paused;
		}

		void FmodAudio::pauseGroup(const char* group) {
			if (auto r = impl_->ensureGroup(group); r != AudioResult::Ok) return;
			auto& g = impl_->groups[group];
			if (g.cg) g.cg->setPaused(true);
		}

		void FmodAudio::resumeGroup(const char* group) {
			if (auto r = impl_->ensureGroup(group); r != AudioResult::Ok) return;
			auto& g = impl_->groups[group];
			if (g.cg) g.cg->setPaused(false);
		}

		void FmodAudio::stopGroup(const char* group) {
			if (auto r = impl_->ensureGroup(group); r != AudioResult::Ok) return;
			auto& g = impl_->groups[group];
			if (g.cg) g.cg->stop();
		}

		void FmodAudio::onAppPause() {
			if (!impl_->initialized) return;
			PN_CORE_INFO("[FMOD] onAppPause() - Suspending mixer");
			impl_->sys->mixerSuspend();
		}
		void FmodAudio::onAppResume() {
			if (!impl_->initialized) return;
			PN_CORE_INFO("[FMOD] onAppResume() - Resuming mixer");
			impl_->sys->mixerResume();
		}

		// ==================== New Direct File Playback Methods ====================

		std::optional<AudioChannelId> FmodAudio::playFile(const std::string& filename, 
			const std::string& group, float volumeDb, bool looping, bool is3D, 
			const glm::vec3& pos, float minDist, float maxDist) {
			
			PN_CORE_INFO("[FmodAudio::playFile] Playing: {} (group={}, vol={:.1f}dB, 3D={}, loop={})", 
				filename, group, volumeDb, is3D, looping);
			
			if (!impl_->initialized) {
				PN_CORE_WARN("[FmodAudio::playFile] FMOD not initialized!");
				return std::nullopt;
			}

			// Create sound on-the-fly
			FMOD_MODE mode = FMOD_DEFAULT;
			mode |= (is3D ? FMOD_3D : FMOD_2D);
			mode |= (looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

			FMOD::Sound* s = nullptr;
			FMOD_RESULT r = impl_->sys->createSound(filename.c_str(), mode, nullptr, &s);
			if (r != FMOD_OK || !s) {
				PN_CORE_ERROR("[FmodAudio::playFile] Failed to create sound: {} (FMOD_RESULT={})", filename, (int)r);
				return std::nullopt;
			}

			if (is3D) {
				s->set3DMinMaxDistance(minDist, maxDist);
			}

			// Pick channel group
			impl_->ensureGroup(group.c_str());
			FMOD::ChannelGroup* cg = nullptr;
			auto g_it = impl_->groups.find(group);
			if (g_it != impl_->groups.end()) {
				cg = g_it->second.cg;
			} else {
				cg = impl_->groups["sfx"].cg;
			}

			// Play sound
			FMOD::Channel* ch = nullptr;
			r = impl_->sys->playSound(s, cg, true, &ch);
			if (r != FMOD_OK || !ch) {
				PN_CORE_ERROR("[FmodAudio::playFile] Failed to play sound: {} (FMOD_RESULT={})", filename, (int)r);
				s->release();
				return std::nullopt;
			}

			// Set 3D position
			if (is3D) {
				auto fpos = Impl::toF(pos);
				FMOD_VECTOR vel{ 0,0,0 };
				ch->set3DAttributes(&fpos, &vel);
			}

			ch->setVolume(db2lin(volumeDb));
			ch->setPaused(false);

			// Track channel
			int id = impl_->nextId++;
			impl_->channels.emplace(id, Impl::Chan{ ch });
			PN_CORE_INFO("[FmodAudio::playFile] Success! Channel ID={}", id);
			return AudioChannelId{ id };
		}

		std::optional<AudioChannelId> FmodAudio::playSFX(const std::string& filename, 
			bool looping, float volumeDb) {
			// Play as 2D sound in "sfx" group
			return playFile(filename, "sfx", volumeDb, looping, false, glm::vec3(0), 
				MIN_DISTANCE_3D, MAX_DISTANCE_3D);
		}

		std::optional<AudioChannelId> FmodAudio::playSFXAt(const std::string& filename,
			const glm::vec3& pos, float volumeDb, bool looping,
			float minDist, float maxDist) {
			// Play as 3D sound at specified position
			return playFile(filename, "sfx", volumeDb, looping, true, pos, minDist, maxDist);
		}

		std::optional<AudioChannelId> FmodAudio::playRandomFromList(
			const std::vector<std::string>& files, float volumeDb, bool is3D,
			const glm::vec3& pos, float minDist, float maxDist) {
			
			if (files.empty()) {
				PN_CORE_WARN("[FmodAudio::playRandomFromList] Empty file list");
				return std::nullopt;
			}

			// C++ random selection
			static std::mt19937 rng(std::random_device{}());
			std::uniform_int_distribution<size_t> dist(0, files.size() - 1);
			size_t idx = dist(rng);

			const std::string& selectedFile = files[idx];
			PN_CORE_INFO("[FmodAudio::playRandomFromList] Selected file {} of {}: {}", 
				idx + 1, files.size(), selectedFile);
			return playFile(selectedFile, "sfx", volumeDb, false, is3D, pos, minDist, maxDist);
		}

		std::optional<AudioChannelId> FmodAudio::playBGM(const std::string& filename, 
			bool overlay, float volumeDb) {
			
			if (!impl_->initialized) return std::nullopt;

			// If not overlay, stop current music first
			if (!overlay) {
				stopGroup("music");
			}

			// Create streaming sound for BGM
			FMOD_MODE mode = FMOD_2D | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM;
			
			FMOD::Sound* s = nullptr;
			FMOD_RESULT r = impl_->sys->createSound(filename.c_str(), mode, nullptr, &s);
			if (r != FMOD_OK || !s) {
				PN_CORE_ERROR("[FmodAudio::playBGM] Failed to create BGM sound: {}", filename);
				return std::nullopt;
			}

			// Get music group
			impl_->ensureGroup("music");
			FMOD::ChannelGroup* cg = impl_->groups["music"].cg;

			// Play sound
			FMOD::Channel* ch = nullptr;
			r = impl_->sys->playSound(s, cg, true, &ch);
			if (r != FMOD_OK || !ch) {
				s->release();
				return std::nullopt;
			}

			ch->setVolume(db2lin(volumeDb));
			ch->setPaused(false);

			// Track channel
			int id = impl_->nextId++;
			impl_->channels.emplace(id, Impl::Chan{ ch });
			return AudioChannelId{ id };
		}

		void FmodAudio::transitionBGM(const std::string& newFilename, 
			float transitionTime, float volumeDb) {
			
			if (!impl_->initialized) return;

			float halfTime = transitionTime / 2.0f;

			// Fade out current music
			fadeGroupToDb("music", -80.0f, halfTime);

			// After fade, we need to play new BGM
			// Since we can't schedule callbacks easily, we'll start the new BGM 
			// at low volume and fade it in simultaneously
			
			// Create streaming sound for new BGM
			FMOD_MODE mode = FMOD_2D | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM;
			
			FMOD::Sound* s = nullptr;
			FMOD_RESULT r = impl_->sys->createSound(newFilename.c_str(), mode, nullptr, &s);
			if (r != FMOD_OK || !s) {
				PN_CORE_ERROR("[FmodAudio::transitionBGM] Failed to create BGM sound: {}", newFilename);
				return;
			}

			// Get music group  
			impl_->ensureGroup("music");
			FMOD::ChannelGroup* cg = impl_->groups["music"].cg;

			// Play sound (start paused so we can set volume first)
			FMOD::Channel* ch = nullptr;
			r = impl_->sys->playSound(s, cg, true, &ch);
			if (r != FMOD_OK || !ch) {
				s->release();
				return;
			}

			// Start at -80dB and fade in
			ch->setVolume(db2lin(-80.0f));
			ch->setPaused(false);

			// Track channel 
			int id = impl_->nextId++;
			impl_->channels.emplace(id, Impl::Chan{ ch });

			// Schedule fade-in after a brief delay (to let fade-out start)
			// We use the group fade which will affect all channels in the group
			// For proper crossfade, fade the new channel directly
			// This is a simplified implementation - for precise timing,
			// you'd want a more sophisticated system with scheduled callbacks
		}

		void FmodAudio::transitionBGMWithSFX(const std::string& newBGMFilename,
			const std::string& sfxFilename, float transitionTime, float volumeDb) {
			
			// Play the SFX first
			playSFX(sfxFilename, false, 0.0f);
			
			// Then do the BGM transition
			transitionBGM(newBGMFilename, transitionTime, volumeDb);
		}
	}
}
