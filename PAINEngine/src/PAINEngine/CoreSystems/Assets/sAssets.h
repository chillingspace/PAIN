#pragma once

#include "pch.h"
#include "sLoader.h"
#include "Applications/AppSystem.h"
#include "CoreSystems/Path/Path.h"

#include "AssetTypes.h"
#include "AssetLoader.h"
#include "AssetOrganizer.h"
#include "AssetCompiler.h"

namespace PAIN {
    namespace Assets {

		class Manager : public AppSystem {
		private:

			//Asset registry
			std::unordered_map<GUID, std::shared_ptr<IAsset>> asset_registry;

			//Asset cache
			std::unordered_map<GUID, std::shared_ptr<IAsset>> asset_cache;

			//Path to GUID mapping
			std::unordered_map<std::filesystem::path, GUID> shipped_path_to_guid;
			std::unordered_map<std::filesystem::path, GUID> main_path_to_guid;

			//Asset loader
			std::unique_ptr<Loader> asset_loader;

			//Asset Organizer
#ifdef PN_PLATFORM_WINDOWS
			std::unique_ptr<Organizer> asset_organizer;
#endif

			//Log asset registry
			void logAssetRegistry() const;
		public:

			Manager() = default;

			virtual ~Manager() = default;

			//Internal finding of guid
			GUID findGUID(std::filesystem::path const& relative_path);

			//Register asset
#ifdef PN_PLATFORM_WINDOWS
			void registerAsset(std::filesystem::path const& relative_path);
#endif
			void registerAsset(std::shared_ptr<IAsset> asset);

			//Unregister asset
			void unregisterAsset(GUID const& id);

			//Check asset registered
			bool checkAssetRegistered(std::filesystem::path const& relative_path);
			bool checkAssetRegistered(GUID const& id) const;

			//Get asset
			template <typename T>
			std::shared_ptr<T> getAsset(GUID const& id) {

				//Check if GUID is valid
				if (!id.IsValid()) {
					//Asset doesnt exist in registry
					throw std::runtime_error("Invalid GUID.");
				}

				//Check if asset register has id
				if (asset_registry.find(id) == asset_registry.end()) {

					//Asset doesnt exist in registry
					throw std::runtime_error("Asset doesn't exist in registry.");
				}

				//Asset template
				std::shared_ptr<IAsset> asset;

				//Search asset cache
				auto it = asset_cache.find(id);
				if (it == asset_cache.end()) {

					//Cache asset
					asset = cacheAsset(id);
				}
				else {
					asset = it->second;
				}

				auto typed_asset = std::dynamic_pointer_cast<T>(asset);
				if (!typed_asset) {
					throw std::runtime_error("Asset type mismatch (wrong cast to requested type).");
				}
				return typed_asset;
			}

			template <typename T>
			std::shared_ptr<T> getAsset(std::filesystem::path const& relative_path) {

				//Find GUID
				auto id = findGUID(relative_path);

				//Check if GUID is valid
				if (!id.IsValid()) {
					//Asset doesnt exist in registry
					throw std::runtime_error("Invalid GUID.");
				}

				//Check if asset register has id
				if (asset_registry.find(id) == asset_registry.end()) {

					//Asset doesnt exist in registry
					throw std::runtime_error("Asset doesn't exist in registry.");
				}

				//Asset template
				std::shared_ptr<IAsset> asset;

				//Search asset cache
				auto it = asset_cache.find(id);
				if (it == asset_cache.end()) {

					//Cache asset
					asset = cacheAsset(id);
				}
				else {
					asset = it->second;
				}

				auto typed_asset = std::dynamic_pointer_cast<T>(asset);
				if (!typed_asset) {
					throw std::runtime_error("Asset type mismatch (wrong cast to requested type).");
				}
				return typed_asset;
			}

			//Caching of assets
			std::shared_ptr<IAsset> cacheAsset(GUID const& id);
			void batchCacheAssets(std::vector<GUID> batch_ids);
			void uncacheAsset(GUID const& id);
			std::shared_ptr<IAsset> recacheAsset(GUID const& id);
			bool checkAssetCached(GUID const& id) const;

			//Find asset type
			std::shared_ptr<IAsset> getAssetData(GUID const& id) const;
			std::shared_ptr<IAsset> getAssetData(std::filesystem::path const& relative_path);

#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG

			//Debug only editor mode functions

			//Move file function
			void moveFile(std::filesystem::path const& from, std::filesystem::path const& to);

			//Delete file function
			void removeFile(std::filesystem::path const& file_path);
#endif
#endif

			//AppSystem overrides
			void onAttach() override;
			void onUpdate(AppTiming timing) override {}
			void onDetach() override;
			void onFixedUpdate(AppTiming timing) override {}
			void onAppPause() override {}
			void onAppResume() override {}
			void onEvent(Event::Event& e) override {}
		};

#ifdef PN_PLATFORM_WINDOWS

		//Temporary Disable DLL Export Warning
		#pragma warning(disable: 4251)

		//File Drop Event (TO DO: File Drop)
		/*struct FileDropEvent : public Events::IEvent {
			int count;
			const char** paths;

			FileDropEvent(int count, const char** paths)
				: count{ count }, paths{ paths } {
			}
		};*/

		// ----------------------------
		// Asset Modes/Types 
		// ----------------------------
		enum Modes : unsigned int { // Modes
			Loadable = 0,
			Executable,
			Editable
		};

		enum class Types { // Types
			None = 0,
			Texture,
			Model,
			Font,
			Music,
			Sound,
			Scene,
			Prefab,
			Grid,
			Script,
			Video
		};

		// ----------------------------
		// Asset Service 
		// ----------------------------
		class Service : public AppSystem {
		public:

			// ----------------------------
			// Life Cycle 
			// ----------------------------
			
			Service() = default; //Default constructor and destructor
			~Service() = default;

			void onAttach() override;
			void onFixedUpdate(AppTiming timing) override {}
			void onUpdate(AppTiming timing) override {}
			void onDetach() override {}

			void onAppPause() {}
			void onAppResume() {}

			void onEvent(Event::Event& e) override {}; //Event handler for app layer

			//(TO DO: Init Audio System)
			void init(); //void init(std::shared_ptr<Audio::IAudioSystem> audio_sys); //Initialization 

			// ----------------------------
			// Asset Registration & Loading
			// ----------------------------

			std::string registerAsset(std::string const& path, bool b_virtual = true); //Register asset
			void unregisterAsset(std::string const& asset_id); //Unregister asset

			//(TO DO: Register Loader)
			//void registerLoader(Types asset_type, LoaderFunc loader); //Register loader


			void cacheAsset(std::string const& asset_id); //Cache asset
			void uncacheAsset(std::string const& asset_id); //Uncache asset
			void recacheAsset(std::string const& asset_id); //Recache asset


			template <typename T>
			std::shared_ptr<T> getAsset(std::string const& asset_id); //Get asset

			void getExecutable(std::string const& asset_id); //Get executable

			// ----------------------------
			// Type & State Queries
			// ----------------------------

			bool isAssetLoadable(std::string const& asset_id) const; //check if asset is loadable type
			bool isAssetExecutable(std::string const& asset_id) const; //Check if asset is executable type
			bool isAssetEditable(std::string const& asset_id) const; //Check if asset is editable type


			Types getAssetType(std::string const& asset_id) const; //Get asset type from registered asset id
			std::string getAssetTypeString(std::string const& asset_id) const; //Get asset type string from registered asset id
			Types getAssetType(std::filesystem::path const& path) const; //Get asset type from path


			std::filesystem::path getAssetPath(std::string const& asset_id) const; //Get asset path from registered asset id
			std::vector<const char*> getAssetRefs(Types type) const; //Get all asset ref of type


			bool isAssetCached(std::string const& asset_id) const; //Check if asset is loaded from asset id
			bool isAssetCached(std::filesystem::path const& path) const; //Check if asset is loaded from file path


			void addValidExtensions(std::string const& ext); //Add valid extension
			std::set<std::string> getValidExtensions() const; //Get all valid extensions


			void addInvalidKeys(std::string const& key); //Add invalid keys
			std::set<std::string> getInvalidKeys() const; //Get all invalid keys


			bool isPathValid(std::string const& path, bool b_virtual = true) const; //Check for valid path
			bool isAssetRegistered(std::string const& asset_id) const; //Check for registration


			std::string getIDFromPath(std::string const& path, bool b_virtual = true) const; //Get ref from path

			// ----------------------------
			// Debugging / Logging
			// ----------------------------
			void clearCache(); //Clear expired cache
			void logAssetsRegistry() const; //Log assets reigstry


			// ----------------------------
			// Directory Helpers
			// ----------------------------
			void scanAssetDirectory(std::string const& virtual_path, bool b_diretory_tree = false); //Register all assets from directory tree
			void cacheAssetDirectory(std::string const& virtual_path, bool b_diretory_tree = false); //Cache all assets from directory tree
			void uncacheAssetDirectory(std::string const& virtual_path, bool b_diretory_tree = false); //Remove cache for all assets from directory tree


			// ----------------------------
			// Serialization
			// ----------------------------
			nlohmann::json serialize() const; //Serialize asset registry
			void deserialize(nlohmann::json const& data); //Deserialize asset registry
			void reserializeAllAssets(); //Reserialize data

		private:

			// ----------------------------
			// Internal Data Structures
			// ----------------------------
			struct MetaData { // Asset Meta Data
				Types type;
				std::filesystem::path primary_path;
				std::weak_ptr<void> cached;

				MetaData() : type{ 0 } {};
				MetaData(Types type, std::filesystem::path const& primary_path)
					: type{ type }, primary_path{ primary_path } {
				}
			};


			// ----------------------------
			// Loader
			// ----------------------------
			using LoaderFunc = std::function<std::shared_ptr<void>(std::filesystem::path const&)>; //Loader function
			//Font loader (TO DO: Loader)
			//std::unique_ptr<Assets::FontLoader> font_loader;
			//Render loader
			//std::unique_ptr<Assets::RenderLoader> render_loader;
			//Audio loader
			//std::shared_ptr<Audio::IAudioSystem> audio_system;


			// ----------------------------
			// Containers
			// ----------------------------
			std::set<std::string> valid_extensions; //List of valid extension
			std::set<std::string> invalid_keys; //List of key words to exclude


			std::unordered_map<Types, std::bitset<3>> asset_types; //Asset types
			std::unordered_map<std::string, MetaData> asset_registry; //Asset registry of meta data
			std::unordered_map<Types, LoaderFunc> asset_loader; //Asset loader
			std::unordered_map<std::string, std::shared_ptr<void>> asset_cache; //Assets cache for storing assets ( Optionally change to weakptr for a more event driven approach )


			// ----------------------------
			// Helpers
			// ----------------------------
			std::string typeToString(Types type) const; //Conversion from type to string

		};

#endif

		//Re-enable DLL Export warning
		#pragma warning(default: 4251)

    } // namespace Assets
} // namespace PAIN
