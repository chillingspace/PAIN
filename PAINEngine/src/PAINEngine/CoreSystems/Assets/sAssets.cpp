
#include "pch.h"
#include "Applications/Application.h"
#include "sAssets.h"
#include "sPath.h"

#define PN_PATH_SERVICE  services->get<Path::Service>()
#define PN_LOADER_SERVICE  services->get<Loader::Service>()

namespace PAIN {
	namespace Assets {

		void Manager::logAssetRegistry() const {

			for (auto const& asset : asset_registry) {
				PN_CORE_INFO("GUID: {} Path: {}", asset.first.ToString(), asset.second->shipped_relative_path.string());
			}
		}

		GUID Manager::findGUID(std::filesystem::path const& relative_path) {

			//Normalize path first
			auto path_service = services->get<Path::Path>();
			std::filesystem::path norm_path = path_service->normalizePath(relative_path.string());
			
			//Find GUID
			auto main_it = main_path_to_guid.find(norm_path);
			if (main_it != main_path_to_guid.end()) {
				return main_it->second;
			}
			else {
				auto shipped_it = shipped_path_to_guid.find(norm_path);
				if (shipped_it != shipped_path_to_guid.end()) {
					return shipped_it->second;
				}
			}

			return GUID();
		}

		void Manager::onAttach() {

			//Get path service
			auto path_service = services->get<Path::Path>();

			//Create unique asset loader
			asset_loader = std::make_unique<Loader>(services);

#ifdef PN_PLATFORM_WINDOWS
			Platform platform = Platform::Windows;

			//Create unique asset compiler
			asset_organizer = std::make_unique<Organizer>(path_service->resolvePath(Path::main_assets_alias, ""),
				path_service->resolvePath(Path::assets_alias, ""),
				platform, getExecutablePath());
#endif

			//Register texture loader
			asset_loader->RegisterLoader(Type::Texture, [this](std::string const& virtual_path) {

				return asset_loader->ImportTexture(virtual_path);
				});

			//Register Model loader
			asset_loader->RegisterLoader(Type::Model, [this](std::string const& virtual_path) {

				return asset_loader->ImportModel(virtual_path);
				});

			//Register Model loader
			asset_loader->RegisterLoader(Type::Shader, [this](std::string const& virtual_path) {

				//Get path service
				auto path_service = services->get<Path::Path>();

				//Get alias & relative
				auto relative_path = path_service->getRelative(virtual_path);
				auto alias = path_service->getAlias(virtual_path);

				//Craft vert & frag
				auto vert = std::filesystem::path(relative_path).replace_extension(".vert");
				auto frag = std::filesystem::path(relative_path).replace_extension(".frag");

				//Check for shader has already been cached in vert
				auto vert_id = findGUID(vert);
				auto vert_it = asset_cache.find(vert_id);
				if (vert_it != asset_cache.end()) {
					return std::dynamic_pointer_cast<Shader>(vert_it->second);
				}

				//Check for shader has already been cached in frag
				auto frag_id = findGUID(frag);
				auto frag_it = asset_cache.find(frag_id);
				if (frag_it != asset_cache.end()) {
					return std::dynamic_pointer_cast<Shader>(frag_it->second);
				}

				//Craft frag and vert virtual paths
				auto virtual_vert = path_service->aliasCombineRelative(alias, vert.string());
				auto virtual_frag = path_service->aliasCombineRelative(alias, frag.string());

				//Else import shader
				return asset_loader->ImportShader(virtual_vert, virtual_frag);
				});

			//Import asset registry
			asset_registry = asset_loader->ImportAssetRegistry("assets://" + asset_registry_filename);

			//Instantiate the path to guid
			for (auto const& asset : asset_registry) {
                std::filesystem::path norm_shipped = path_service->normalizePath(asset.second->shipped_relative_path.string());
                std::filesystem::path norm_main = path_service->normalizePath(asset.second->main_relative_path.string());
				shipped_path_to_guid[norm_shipped] = asset.second->guid;
				main_path_to_guid[norm_main] = asset.second->guid;
			}

			//Dump asset registry
			logAssetRegistry();
		}

		void Manager::onDetach() {
			asset_loader = nullptr;
		}

#ifdef PN_PLATFORM_WINDOWS
		void Manager::registerAsset(std::filesystem::path const& relative_path) {
			//Get path service
			auto path_service = services->get<Path::Path>();

			//Double check to ensure asset is not registered
			if (checkAssetRegistered(findGUID(relative_path))) return;

			//Process asset
			auto asset = asset_organizer->organizeAndProcessAsset(path_service->resolvePath(Path::main_assets_alias, relative_path.string()));

			//Check to ensure that GUID is valid
			if (!asset.guid.IsValid()) return;

			//Get asset registry data
			asset_registry[asset.guid] = std::make_shared<IAsset>(asset);
		}
#endif

		void Manager::registerAsset(std::shared_ptr<IAsset> asset) {

			//Check to ensure that GUID is valid
			if (!asset->guid.IsValid()) return;

			//Get asset registry data
			asset_registry[asset->guid] = asset;
		}

		void Manager::unregisterAsset(GUID const& id) {

			//Find cache it and uncache
			auto cahce_it = asset_cache.find(id);
			if (cahce_it != asset_cache.end()) {
				cahce_it = asset_cache.erase(cahce_it);
			}

			//Get asset registry data
			auto registry_it = asset_registry.find(id);
			if (registry_it != asset_registry.end()) {
				registry_it = asset_registry.erase(registry_it);
			}
		}

		bool Manager::checkAssetRegistered(GUID const& id) const {
			auto registry_it = asset_registry.find(id);
			if (registry_it != asset_registry.end()) {
				return true;
			}

			return false;
		}

		bool Manager::checkAssetRegistered(std::filesystem::path const& relative_path) {

			//Find GUID
			auto id = findGUID(relative_path);

			auto registry_it = asset_registry.find(id);
			if (registry_it != asset_registry.end()) {
				return true;
			}

			return false;
		}

		std::shared_ptr<IAsset> Manager::cacheAsset(GUID const& id) {

			//Get asset registry data
			auto registry_it = asset_registry.find(id);
			if (registry_it == asset_registry.end()) {
				throw std::runtime_error("Assset that does not exist in the registry! Unable to cache!");
			}

			//Check asset cache
			auto cache_it = asset_cache.find(id);
			if (cache_it != asset_cache.end()) {
				return cache_it->second;
			}

			//Resolve asset path
			auto virtual_path = services->get<Path::Path>()->aliasCombineRelative("assets", registry_it->second->shipped_relative_path.string());

			//Load assset through registered loaded
			auto asset = asset_loader->GetLoader(registry_it->second->type)(virtual_path);

			//Insert loaded asset into asset cache
			asset_cache.emplace(id, asset);

			return asset;
		}

		void Manager::batchCacheAssets(std::vector<GUID> batch_ids) {

			//Get all batch ids
			for (auto id : batch_ids) {

				//check registration
				if (checkAssetRegistered(id)) {

					//Check asset cache
					if (!checkAssetCached(id)) {
						
						//Cache asset
						cacheAsset(id);
					}
				}
			}
		}

		void Manager::uncacheAsset(GUID const& id) {
			//Check asset cache
			auto cache_it = asset_cache.find(id);
			if (cache_it != asset_cache.end()) {
				cache_it = asset_cache.erase(cache_it);
			}
		}

		std::shared_ptr<IAsset> Manager::recacheAsset(GUID const& id) {
			//Uncache asset
			uncacheAsset(id);

			//Cache asset
			return cacheAsset(id);
		}

		bool Manager::checkAssetCached(GUID const& id) const {
			//Check asset cache
			auto cache_it = asset_cache.find(id);
			if (cache_it != asset_cache.end()) {
				return true;
			}

			return false;
		}

		std::shared_ptr<IAsset> Manager::getAssetData(GUID const& id) const {
			//Get asset registry data
			auto registry_it = asset_registry.find(id);
			if (registry_it == asset_registry.end()) {
				throw std::runtime_error("Assset that does not exist in the registry! Unable to cache!");
			}

			return std::make_shared<IAsset>(*registry_it->second);
		}
		
		std::shared_ptr<IAsset> Manager::getAssetData(std::filesystem::path const& relative_path) {
			//Find GUID
			auto id = findGUID(relative_path);

			//Check if GUID is valid
			if (!id.IsValid()) {
				//Asset doesnt exist in registry
				throw std::runtime_error("Invalid GUID.");
			}

			//Get asset registry data
			auto registry_it = asset_registry.find(id);
			if (registry_it == asset_registry.end()) {
				throw std::runtime_error("Assset that does not exist in the registry! Unable to cache!");
			}

			return std::make_shared<IAsset>(*registry_it->second);
		}

#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG
		void Manager::moveFile(std::filesystem::path const& from, std::filesystem::path const& to) const {

			//Move file
			asset_organizer->moveFile(from, to);
		}

		void Manager::removeFile(std::filesystem::path const& file_path) const {

			//Remove file
			asset_organizer->removeFile(file_path);
		}
#endif
#endif
		// ----------------------------
		// Asset Service 
		// ----------------------------

#ifdef PN_PLATFORM_WINDOWS
		void Service::onAttach() {
			/*font_loader = std::make_unique<Assets::FontLoader>();
			render_loader = std::make_unique<Assets::RenderLoader>();
			audio_system = audio_sys;*/

			//Set loadable
			asset_types[Types::Texture].set(Modes::Loadable, true);
			asset_types[Types::Model].set(Modes::Loadable, true);
			asset_types[Types::Font].set(Modes::Loadable, true);
			asset_types[Types::Music].set(Modes::Loadable, true);
			asset_types[Types::Sound].set(Modes::Loadable, true);
			asset_types[Types::Script].set(Modes::Loadable, true);
			asset_types[Types::Video].set(Modes::Loadable, true);

			//Set executables
			asset_types[Types::Scene].set(Modes::Executable, true);
			asset_types[Types::Prefab].set(Modes::Executable, true);
			asset_types[Types::Grid].set(Modes::Executable, true);

			//Set editable
			asset_types[Types::Scene].set(Modes::Editable, true);
			asset_types[Types::Prefab].set(Modes::Editable, true);
			asset_types[Types::Grid].set(Modes::Editable, true);
			asset_types[Types::Script].set(Modes::Editable, true);

			//Texture extensions
			addValidExtensions(".png");
			addValidExtensions(".jpg");
			addValidExtensions(".jpeg");
			addValidExtensions(".tex");

			//Font extension
			addValidExtensions(".ttf");

			//Model extension
			addValidExtensions(".model");

			//Audio extension
			addValidExtensions(".wav");

			//Video extension
			addValidExtensions(".mpg");

			//Other extension
			addValidExtensions(".prefab");
			addValidExtensions(".scn");
			addValidExtensions(".grid");
			addValidExtensions(".lua");
			addValidExtensions(".json");

			//Add invalid keys
			addInvalidKeys("batched_");

			//Register texture loader
			//registerLoader(Assets::Types::Texture, [this](std::filesystem::path const& primary_path) {
			//	return std::make_shared<Texture>(render_loader->compileTexture(primary_path.string()));
			//	});

			////Register model loader
			//registerLoader(Assets::Types::Model, [this](std::filesystem::path const& primary_path) {
			//	return std::make_shared<Model>(render_loader->compileModel(primary_path.string()));
			//	});

			////Register font loader
			//registerLoader(Assets::Types::Font, [this](std::filesystem::path const& primary_path) {
			//	return std::make_shared<Font>(std::static_pointer_cast<Assets::NIKEFontLib>(font_loader->getFontLib())->generateFont(primary_path.string()));
			//	});

			////Register music loader
			//registerLoader(Assets::Types::Music, [this](std::filesystem::path const& primary_path) {
			//	return audio_system->createStream(primary_path.string());
			//	});

			////Register Sound loader
			//registerLoader(Assets::Types::Sound, [this](std::filesystem::path const& primary_path) {
			//	return audio_system->createSound(primary_path.string());
			//	});

			////Register Scene loader
			//registerLoader(Assets::Types::Scene, [this](std::filesystem::path const& primary_path) {
			//	NIKE_SERIALIZE_SERVICE->loadSceneFromFile(primary_path.string());
			//	return nullptr;
			//	});

			////Register Prefab loader
			//registerLoader(Assets::Types::Prefab, [this](std::filesystem::path const& primary_path) {
			//	// Temp entity for the prefab loading
			//	Entity::Type temp_entity = NIKE_ECS_MANAGER->createEntity();
			//	NIKE_SERIALIZE_SERVICE->loadEntityFromFile(temp_entity, primary_path.string());
			//	return nullptr;
			//	});

			////Register Scripts loader
			//registerLoader(Assets::Types::Script, [this](std::filesystem::path const& primary_path) {
			//	return std::make_shared<sol::load_result>(NIKE_LUA_SERVICE->loadScript(primary_path));
			//	});
		}


		// ----------------------------
		// Asset Registration & Loading
		// ----------------------------
		std::string Service::registerAsset(std::string const& path, bool b_virtual) {

			if (b_virtual) {
				if (!isPathValid(path)) {
					PN_CORE_WARN("Invalid path detected. Asset will not be registered.");
					return "";
				}
				auto asset_id = getIDFromPath(path);
				auto asset_type = getAssetType(PN_PATH_SERVICE->resolvePath(path));
				asset_registry[asset_id] = MetaData(asset_type, PN_PATH_SERVICE->resolvePath(path));

				return asset_id;
			}
			else {
				if (!isPathValid(path, false)) {
					PN_CORE_WARN("Invalid path detected. Asset will not be registered.");
					return "";
				}
				auto asset_id = getIDFromPath(path, false);
				auto asset_type = getAssetType(std::filesystem::path(path));
				asset_registry[asset_id] = MetaData(asset_type, path);

				return asset_id;
			}
		}

		void Service::unregisterAsset(std::string const& asset_id) {

			//Check asset registry
			auto register_it = asset_registry.find(asset_id);
			if (register_it != asset_registry.end()) {
				//Unregister
				register_it = asset_registry.erase(register_it);
				//Uncache
				asset_cache.erase(asset_id);
			}
		}

		/*void Service::registerLoader(Types asset_type, LoaderFunc loader) {
			if (asset_loader.find(asset_type) != asset_loader.end()) {
				throw std::runtime_error("Loader already registered.");
			}

			asset_loader.emplace(asset_type, loader);
		}*/


		void Service::cacheAsset(std::string const& asset_id) {

			//Check if asset is loadable
			if (!isAssetLoadable(asset_id)) {
				return;
			}

			//Check asset cache
			auto cache_it = asset_cache.find(asset_id);
			if (cache_it != asset_cache.end()) {
				return;
			}

			//Get asset meta data
			auto meta_it = asset_registry.find(asset_id);
			if (meta_it == asset_registry.end()) {
				return;
			}

			//Load assset through registered loaded
			auto loader_it = asset_loader.find(meta_it->second.type);
			if (loader_it == asset_loader.end()) {
				return;
			}

			//Get loaded asset
			auto asset = loader_it->second(meta_it->second.primary_path);

			//Insert loaded asset into asset cache
			asset_cache.emplace(asset_id, asset);
		}

		void Service::uncacheAsset(std::string const& asset_id) {

			//Check if asset is loadable
			if (!isAssetLoadable(asset_id)) {
				return;
			}

			//Check asset cache
			auto cache_it = asset_cache.find(asset_id);
			if (cache_it != asset_cache.end()) {
				cache_it = asset_cache.erase(cache_it);
			}
		}

		void Service::recacheAsset(std::string const& asset_id) {

			//Check if asset is loadable
			if (!isAssetLoadable(asset_id)) {
				return;
			}

			//Uncache asset
			uncacheAsset(asset_id);

			//Cache asset
			cacheAsset(asset_id);
		}

		template <typename T>
		std::shared_ptr<T> Service::getAsset(std::string const& asset_id) {

			//Check if asset is a executable asset type
			if (!(asset_types[getAssetType(asset_id)].test(Modes::Loadable))) {
				return nullptr;
			}

			//Check asset cache
			auto cache_it = asset_cache.find(asset_id);
			if (cache_it != asset_cache.end()) {
				if (cache_it->second) {
					return std::static_pointer_cast<T>(cache_it->second);
				}
			}

			//Get asset meta data
			auto meta_it = asset_registry.find(asset_id);
			if (meta_it == asset_registry.end()) {

				//Return nullptr
				return nullptr;
			}

			//Load assset through registered loaded
			auto loader_it = asset_loader.find(meta_it->second.type);
			if (loader_it == asset_loader.end()) {

				//Return nullptr
				throw std::runtime_error("Loader not registered for asset type");
			}

			//Get loaded asset
			auto asset = loader_it->second(meta_it->second.primary_path);

			//Insert loaded asset into asset cache
			asset_cache.emplace(asset_id, asset);

			//Return asset
			return std::static_pointer_cast<T>(asset);
		}

		void Service::getExecutable(std::string const& asset_id) {

			//Check if asset is a executable asset type
			if (!(asset_types[getAssetType(asset_id)].test(Modes::Executable))) {
				PN_CORE_WARN("Wrong usage! For fetching normal type assets use getAsset<T>().");
				return;
			}

			//Get asset meta data
			auto meta_it = asset_registry.find(asset_id);
			if (meta_it == asset_registry.end()) {
				return;
			}

			//Load assset through registered loaded
			auto loader_it = asset_loader.find(meta_it->second.type);
			if (loader_it == asset_loader.end()) {

				//Return nullptr
				throw std::runtime_error("Loader not registered for asset type");
			}

			//Run executable
			loader_it->second(meta_it->second.primary_path);
		}


		// ----------------------------
		// Type & State Queries
		// ----------------------------

		bool Service::isAssetLoadable(std::string const& asset_id) const {
			return asset_types.at(getAssetType(asset_id)).test(Modes::Loadable);
		}

		bool Service::isAssetExecutable(std::string const& asset_id) const {
			return asset_types.at(getAssetType(asset_id)).test(Modes::Executable);
		}

		bool Service::isAssetEditable(std::string const& asset_id) const {
			return asset_types.at(getAssetType(asset_id)).test(Modes::Editable);
		}

		Types Service::getAssetType(std::string const& asset_id) const {
			if (asset_registry.find(asset_id) == asset_registry.end()) {
				return Types::None;
			}

			return asset_registry.at(asset_id).type;
		}

		std::string Service::getAssetTypeString(std::string const& asset_id) const {
			if (asset_registry.find(asset_id) == asset_registry.end()) {
				return typeToString(Types::None);
			}

			return typeToString(asset_registry.at(asset_id).type);
		}

		Types Service::getAssetType(std::filesystem::path const& path) const {
			auto ext = path.extension().string();
			// constexpr size_t music_threshold = 5 * 1024 * 1024; // 5 MB
			if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tex") {
				return Types::Texture;
			}
			else if (ext == ".model") {
				return Types::Model;
			}
			else if (ext == ".ttf") {
				return Types::Font;
			}
			else if (ext == ".wav") {
				auto filepath = path.string();
				std::transform(filepath.begin(), filepath.end(), filepath.begin(), [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });

				if (filepath.find("assets\\audios\\music") != std::string::npos) {
					return Types::Music;
				}
				else {
					return Types::Sound;
				}
			}
			else if (ext == ".scn") {
				return Types::Scene;
			}
			else if (ext == ".prefab") {
				return Types::Prefab;
			}
			else if (ext == ".grid") {
				return Types::Grid;
			}
			else if (ext == ".lua") {
				return Types::Script;
			}
			else if (ext == ".mpg") {
				return Types::Video;
			}
			else {
				return Types::None;
			}
		}

		std::filesystem::path Service::getAssetPath(std::string const& asset_id) const {
			if (asset_registry.find(asset_id) == asset_registry.end()) {
				throw std::runtime_error("Asset not yet registered.");
			}

			return asset_registry.at(asset_id).primary_path;
		}

		std::vector<const char*> Service::getAssetRefs(Types type) const {
			std::vector<const char*> asset_refs = { "" };
			for (auto it = asset_registry.begin(); it != asset_registry.end(); ++it) {

				//Check if the asset contains any invalid keys
				bool is_invalid = false;
				for (const auto& invalid_key : invalid_keys) {

					//If invalid key is found
					if (it->second.primary_path.filename().string().find(invalid_key) != std::string::npos) {
						is_invalid = true;
						break;
					}
				}

				//Skip invalid keys or mismatched types
				if (is_invalid || it->second.type != type) {
					continue;
				}

				//Push to assets refs
				asset_refs.push_back(it->first.c_str());
			}

			return asset_refs;
		}

		bool Service::isAssetCached(std::string const& asset_id) const {

			//Check asset cache
			auto cache_it = asset_cache.find(asset_id);
			if (cache_it != asset_cache.end()) {
				return true;
			}

			return false;
		}

		bool Service::isAssetCached(std::filesystem::path const& path) const {
			auto id = getIDFromPath(path.string(), false);

			//Check asset cache
			auto cache_it = asset_cache.find(id);
			if (cache_it != asset_cache.end()) {
				return true;
			}

			return false;
		}

		void Service::addValidExtensions(std::string const& ext) {
			valid_extensions.insert(ext);
		}

		std::set<std::string> Service::getValidExtensions() const {
			return valid_extensions;
		}

		void Service::addInvalidKeys(std::string const& key) {
			invalid_keys.insert(key);
		}

		std::set<std::string>Service::getInvalidKeys() const {
			return invalid_keys;
		}

		bool Service::isPathValid(std::string const& path, bool b_virtual) const {
			if (b_virtual) {
				auto ext = PN_PATH_SERVICE->resolvePath(path).extension().string();
				if (valid_extensions.find(ext) != valid_extensions.end()) {
					return true;
				}
				else {
					return false;
				}
			}
			else {
				auto ext = std::filesystem::path(path).extension().string();
				if (valid_extensions.find(ext) != valid_extensions.end()) {
					return true;
				}
				else {
					return false;
				}
			}
		}

		bool Service::isAssetRegistered(std::string const& asset_id) const {
			return asset_registry.find(asset_id) != asset_registry.end();
		}

		std::string Service::getIDFromPath(std::string const& path, bool b_virtual) const {

			if (b_virtual) {
				auto actual_path = PN_PATH_SERVICE->normalizePath(PN_PATH_SERVICE->resolvePath(path)).string();
				//string variables
				size_t start = actual_path.find_last_of('\\') + 1;
				//size_t size = actual_path.find_first_of('.', start) - start;
				std::string asset_id = actual_path.substr(start);

				return asset_id;
			}
			else {
				auto actual_path = PN_PATH_SERVICE->normalizePath(path).string();
				//string variables
				size_t start = actual_path.find_last_of('\\') + 1;
				//size_t size = actual_path.find_first_of('.', start) - start;
				std::string asset_id = actual_path.substr(start);

				return asset_id;
			}
		}

		// ----------------------------
		// Debugging / Logging
		// ----------------------------
		void Service::clearCache() {
			//Clear asset cache when needed
			asset_cache.clear();
		}


		// ----------------------------
		// Directory Helpers
		// ----------------------------
		void Service::scanAssetDirectory(std::string const& virtual_path, bool b_diretory_tree) {

			//Resolve path
			auto root_path = PN_PATH_SERVICE->resolvePath(virtual_path);

			//Scan just root path
			if (!b_diretory_tree) {
				for (const auto& file : std::filesystem::directory_iterator(root_path)) {
					if (!file.is_regular_file()) continue;

					//Check for valid path before registering
					if (valid_extensions.find(file.path().extension().string()) == valid_extensions.end()) {
						PN_CORE_WARN("Asset will not be registered. Invalid extension found: " + file.path().extension().string() + " In path: " + file.path().string());
						continue;
					}

					//Configure asset type
					Types asset_type = getAssetType(file.path());


					//Register asset
					registerAsset(file.path().string(), false);

					//Log registration
					PN_CORE_INFO("Succesfully registered " + getIDFromPath(file.path().string(), false) + " Asset Type: " + typeToString(asset_type));
				}

				return;
			}

			//Scan root & directory tree
			for (const auto& file : std::filesystem::recursive_directory_iterator(root_path)) {
				if (!file.is_regular_file()) continue;

				//Check for valid path before registering
				if (valid_extensions.find(file.path().extension().string()) == valid_extensions.end()) {
					PN_CORE_WARN("Asset will not be registered. Invalid extension found: " + file.path().extension().string() + " In path: " + file.path().string());
					continue;
				}

				//Configure asset type
				Types asset_type = getAssetType(file.path());

				//Register asset
				registerAsset(file.path().string(), false);

				//Log registration
				PN_CORE_INFO("Succesfully registered " + getIDFromPath(file.path().string(), false) + " Asset Type: " + typeToString(asset_type));
			}
		}

		void Service::cacheAssetDirectory(std::string const& virtual_path, bool b_diretory_tree) {

			//Resolve path
			auto root_path = PN_PATH_SERVICE->resolvePath(virtual_path);

			//Load just root path
			if (!b_diretory_tree) {
				for (const auto& file : std::filesystem::directory_iterator(root_path)) {
					if (!file.is_regular_file()) continue;

					//Check for valid path before caching
					if (valid_extensions.find(file.path().extension().string()) == valid_extensions.end()) {
						continue;
					}

					//Get asset id
					auto asset_id = getIDFromPath(file.path().string(), false);

					//Cache asset
					cacheAsset(asset_id);
				}

				return;
			}

			//Load root & directory tree
			for (const auto& file : std::filesystem::recursive_directory_iterator(root_path)) {
				if (!file.is_regular_file()) continue;

				//Check for valid path before caching
				if (valid_extensions.find(file.path().extension().string()) == valid_extensions.end()) {
					continue;
				}

				//Get asset id
				auto asset_id = getIDFromPath(file.path().string(), false);

				//Cache asset
				cacheAsset(asset_id);
			}
		}

		void Service::uncacheAssetDirectory(std::string const& virtual_path, bool b_diretory_tree) {

			//Resolve path
			auto root_path = PN_PATH_SERVICE->resolvePath(virtual_path);

			//Load just root path
			if (!b_diretory_tree) {
				for (const auto& file : std::filesystem::directory_iterator(root_path)) {
					if (!file.is_regular_file()) continue;

					//Check for valid path before uncaching
					if (valid_extensions.find(file.path().extension().string()) == valid_extensions.end()) {
						continue;
					}

					//Get asset id
					auto asset_id = getIDFromPath(file.path().string(), false);

					//Uncache asset
					uncacheAsset(asset_id);
				}

				return;
			}

			//Load root & directory tree
			for (const auto& file : std::filesystem::recursive_directory_iterator(root_path)) {
				if (!file.is_regular_file()) continue;

				//Check for valid path before uncaching
				if (valid_extensions.find(file.path().extension().string()) == valid_extensions.end()) {
					continue;
				}

				//Get asset id
				auto asset_id = getIDFromPath(file.path().string(), false);

				//Uncache asset
				uncacheAsset(asset_id);
			}
		}

		void Service::logAssetsRegistry() const {
			for (auto const& asset : asset_registry) {
				PN_CORE_INFO("ID: {} Path: {}", asset.first, asset.second.primary_path.string());
			}
		}

		nlohmann::json Service::serialize() const {
			nlohmann::json data;

			//Serialize registry meta data
			for (const auto& [id, metadata] : asset_registry) {
				data[id] = {
							{"Type", static_cast<int>(metadata.type)},
							{"Primary_Path", metadata.primary_path.string()},
				};
			}

			return data;
		}

		void Service::deserialize(nlohmann::json const& data) {

			//Deserialize registry meta data
			for (const auto& [id, meta_data] : data.items()) {
				asset_registry[id] = MetaData(static_cast<Types>(meta_data["Type"].get<int>()), meta_data["Primary_Path"].get<std::string>());
			}
		}

		void Service::reserializeAllAssets() {

			/*Reserialize all assets
			for (auto const& asset_data : asset_registry) {
				try {
					switch (asset_data.second.type) {
					case Types::Prefab: {

						//Create maximum number of layers
						while (PN_SCENES_SERVICE->getLayerCount() < Scenes::MAXLAYERS) {
							PN_SCENES_SERVICE->createLayer();
						}

						//Create tempe entity
						auto temp = PN_ECS_MANAGER->createEntity();

						//Deserialize
						PN_SERIALIZE_SERVICE->loadEntityFromPrefab(temp, asset_data.first);

						//Serialize
						const auto comps = PN_ECS_MANAGER->getAllEntityComponents(temp);
						PN_SERIALIZE_SERVICE->savePrefab(comps, asset_data.second.primary_path.string(), PN_METADATA_SERVICE->getEntityDataCopy(temp).has_value() ? NIKE_METADATA_SERVICE->getEntityDataCopy(temp).value() : NIKE::MetaData::EntityData());

						break;
					}
					case Types::Scene: {
						//Clear scene here
						PN_SCENES_SERVICE->resetScene();

						//Deserialize
						PN_SERIALIZE_SERVICE->loadSceneFromFile(asset_data.second.primary_path.string());

						//Serialize
						PN_SERIALIZE_SERVICE->saveSceneToFile(asset_data.second.primary_path.string());

						//Clear scene here
						PN_SCENES_SERVICE->resetScene();

						break;
					}
					case Types::Grid: {
						//Json Data
						nlohmann::json data;

						//Open file stream
						std::fstream in_file(asset_data.second.primary_path, std::ios::in);

						//Read data from file
						in_file >> data;

						//Deserialize
						PN_MAP_SERVICE->deserialize(data);

						//Open file stream
						std::fstream out_file(asset_data.second.primary_path, std::ios::out | std::ios::trunc);

						//Store data
						out_file << NIKE_MAP_SERVICE->serialize().dump(4);

						break;
					}
					}
				}
				catch ([[maybe_unused]] std::exception const& e) {
					PN_CORE_WARN("Unable to reserialize asset. Deleting asset.");
					std::filesystem::remove(asset_data.second.primary_path);
				}
			}

			//Back to original state
			PN_SCENES_SERVICE->queueSceneEvent(Scenes::SceneEvent(Scenes::Actions::RESTART, ""));*/
		}

		std::string Service::typeToString(Types type) const {
			switch (type) {
			case Types::Texture:
				return "Texture";
				break;
			case Types::Model:
				return "Model";
				break;
			case Types::Font:
				return "Font";
				break;
			case Types::Music:
				return "Music";
				break;
			case Types::Sound:
				return "Sound";
				break;
			case Types::Scene:
				return "Scene";
				break;
			case Types::Prefab:
				return "Prefab";
				break;
			case Types::Grid:
				return "Grid";
				break;
			case Types::Script:
				return "Script";
				break;
			case Types::Video:
				return "Video";
				break;
			default:
				return "Unknown";
				break;
			}
		}

#endif
	}
}
