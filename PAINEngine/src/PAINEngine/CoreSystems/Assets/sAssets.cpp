
#include "pch.h"
#include "Applications/Application.h"
#include "sAssets.h"

#include "CoreSystems/Prefabs/sPrefab.h"
#include "CoreSystems/Assets/Types/Prefab.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/EntityTemplate/sEntityTemplate.h"

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
            //Init platform for organizer
			Platform platform = Platform::Windows;

			//Create unique asset oprganizer
			asset_organizer = std::make_unique<Organizer>(path_service->resolvePath(Path::main_assets_alias, ""),
				path_service->resolvePath(Path::assets_alias, ""),
				platform, getExecutablePath());

			//Create unique asset compiler
			asset_compiler = std::make_unique<Compiler>(path_service->resolvePath(Path::main_assets_alias, ""),
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

			//Register Shader loader
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

			//Register Font loader
			asset_loader->RegisterLoader(Type::Font, [this](std::string const& virtual_path) {

				return asset_loader->ImportFont(virtual_path);
				});

			//Register Audio loader
			asset_loader->RegisterLoader(Type::Audio, [this](std::string const& virtual_path) {

				//Get audio service
				auto audio_service = services->get<Audio::Audio>();
				if (audio_service) {
					return audio_service->createSound(virtual_path);
				}
				else {
					PN_CORE_WARN("Audio service not ready yet, or not initialized.");
					return std::shared_ptr<PAIN::Audio::Sound>();
				}
				});

			//Register Material loader
			asset_loader->RegisterLoader(Type::Material, [this](std::string const& virtual_path) {

				return asset_loader->ImportMaterial(virtual_path);
				});

			//Register Prefab loader
			asset_loader->RegisterLoader(Type::Prefabs, [this](std::string const& virtual_path) {

				auto prefab_service = services->get<Prefab::Service>();

				if (prefab_service) {
					return prefab_service->loadPrefabFromFile(virtual_path);
				}
				else {
					PN_CORE_WARN("Prefab service not ready yet, or not initialized.");
					return std::shared_ptr<PAIN::Prefab::PrefabAsset>();
				}
				});

			//Register Templates loader
			asset_loader->RegisterLoader(Type::Templates, [this](std::string const& virtual_path) {

				auto template_service = services->get<EntityTemplate::Service>();

				if (template_service) {
					return template_service->loadTemplateFromFile(virtual_path);
				}
				else {
					PN_CORE_WARN("Prefab service not ready yet, or not initialized.");
					return std::shared_ptr<PAIN::EntityTemplate::TemplateAsset>();
				}
				});

			//Registry scene loader
			asset_loader->RegisterLoader(Type::Scenes, [this](std::string const& virtual_path) {
				return asset_loader->ImportScene(virtual_path);
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

		GUID Manager::findByName(const std::string& name) 
		{
			GUID g = findGUID(name);
			if (g.IsValid())
				return g;

			// search by asset "name" field from the registry
			for (const auto& [guid, asset] : asset_registry) {
				if (asset && asset->name == name) {
					return guid;
				}
			}

			// Nothing found
			return GUID();
		}

		Type Manager::getTypeByGUID(const GUID& id) const
		{
			auto meta = getAssetData(id);  
			return meta ? meta->type : Type::Other; 
		}

		Loader* Manager::getRawAssetLoader() {
			return asset_loader.get();
		}
		
#ifdef PN_PLATFORM_WINDOWS
		void Manager::registerAsset(std::filesystem::path const& relative_path) {
			//Get path service
			auto path_service = services->get<Path::Path>();

			//Double check to ensure asset is not registered
			if (checkAssetRegistered(findGUID(relative_path))) return;

			//Process asset
			auto asset_vec = asset_organizer->organizeAndProcessAsset(path_service->resolvePath(Path::main_assets_alias, relative_path.string()));

			//Check to ensure asset vec has value
			if (!asset_vec.has_value()) return;

			//Iterate through all assets
			for (auto const& i_asset : asset_vec.value()) {

				//Check for valid guid
				if (!i_asset.guid.IsValid()) return;

				//Get asset registry data
				asset_registry[i_asset.guid] = std::make_shared<IAsset>(i_asset);

				//Register paths to guid
				shipped_path_to_guid[i_asset.shipped_relative_path] = i_asset.guid;
				main_path_to_guid[i_asset.main_relative_path] = i_asset.guid;
			}
		}
#endif

		void Manager::registerAsset(std::shared_ptr<IAsset> asset) {

			//Check to ensure that GUID is valid
			if (!asset->guid.IsValid()) return;

			//Get asset registry data
			asset_registry[asset->guid] = asset;

			//Register paths to guid
			shipped_path_to_guid[asset->shipped_relative_path] = asset->guid;
			main_path_to_guid[asset->main_relative_path] = asset->guid;
		}

		void Manager::unregisterAsset(std::filesystem::path const& relative_path) {

			//Check path if its a directory
			if (relative_path.extension() == "") {

				//Paths to unregister
				std::vector<GUID> unregister_paths;

				//Collect all paths to unregister
				for (const auto& [asset_path, guid] : main_path_to_guid) {
					std::error_code ec;
					auto rel = std::filesystem::relative(asset_path, relative_path, ec);
					if (!rel.empty() && rel.string().find("..") != 0 && ec.value() == 0) {
						unregister_paths.push_back(guid);
					}
				}

				//Unregister each asset
				for (const auto& id : unregister_paths) {
					unregisterAsset(id);
				}
			}
			else {

				//Get guid
				auto id = findGUID(relative_path);
				unregisterAsset(id);
			}

		}

		void Manager::unregisterAsset(GUID const& id) {

			//Find cache it and uncache
			auto cache_it = asset_cache.find(id);
			if (cache_it != asset_cache.end()) {
				cache_it = asset_cache.erase(cache_it);
			}

			//Get asset registry data
			auto registry_it = asset_registry.find(id);
			if (registry_it != asset_registry.end()) {

				//Remove paths to guid
				auto shipped_it = shipped_path_to_guid.find(registry_it->second->shipped_relative_path);
				if (shipped_it != shipped_path_to_guid.end()) {
					shipped_it = shipped_path_to_guid.erase(shipped_it);
				}

				auto main_it = main_path_to_guid.find(registry_it->second->main_relative_path);
				if (main_it != main_path_to_guid.end()) {
					main_it = main_path_to_guid.erase(main_it);
				}

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
			auto virtual_path = services->get<Path::Path>()->aliasCombineRelative(Path::assets_alias, registry_it->second->shipped_relative_path.string());

			//Load asset through registered loaded
			auto asset = asset_loader->GetLoader(registry_it->second->type)(virtual_path);

			//Init asset registry var
			asset->guid = registry_it->second->guid;
			asset->main_relative_path = registry_it->second->main_relative_path;
			asset->shipped_relative_path = registry_it->second->shipped_relative_path;
			asset->name = registry_it->second->name;
			asset->type = registry_it->second->type;

			//Insert loaded asset into asset cache
			asset_cache.emplace(id, asset);

			return asset;
		}

		void Manager::batchCacheAssets(std::unordered_set<GUID> batch_ids) {

			//Get all batch ids
			for (auto const& id : batch_ids) {

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
#ifdef PN_PLATFORM_WINDOWS
		std::shared_ptr<IAsset> Manager::recacheAsset(GUID const& id) {
			//Uncache asset
			uncacheAsset(id);

			//Get path service
			auto path_service = services->get<Path::Path>();

			//Re process asset and ship
			auto data = asset_registry[id];
			Info asset;
			asset.raw_path = path_service->resolvePath(Path::main_assets_alias, "");
			asset.raw_path /= data->main_relative_path;
			asset.name = asset.raw_path.filename().string();
			asset.relative_folder = data->main_relative_path.parent_path();
			asset.type = data->type;
			asset_compiler->processAsset(asset);

			//Cache asset
			return cacheAsset(id);
		}

		void Manager::reshipAsset(GUID const& id) {
			//Get path service
			auto path_service = services->get<Path::Path>();

			//Re process asset and ship
			auto data = asset_registry[id];
			Info asset;
			asset.raw_path = path_service->resolvePath(Path::main_assets_alias, "");
			asset.raw_path /= data->main_relative_path;
			asset.name = asset.raw_path.filename().string();
			asset.relative_folder = data->main_relative_path.parent_path();
			asset.type = data->type;
			asset_compiler->processAsset(asset);
		}
#endif
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

		std::vector <std::shared_ptr<IAsset>> Manager::getAllAssetDataOfType(Type const& type) {

			//Declare temp container
			std::vector<std::shared_ptr<IAsset>> container;

			//Find all assets with type
			for (auto const& asset : asset_registry) {
				if (asset.second->type == type) container.push_back(getAssetData(asset.first));
			}

			//Return all assets
			return container;
		}

#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG
		void Manager::moveFile(std::filesystem::path const& from, std::filesystem::path const& to) {

			//Get path service
			auto path_service = services->get<Path::Path>();
			std::filesystem::path root = path_service->resolvePath(Path::main_assets_alias + path_service->getVirtualSymbol());
			std::filesystem::path relative_from = std::filesystem::relative(from, root);
			std::filesystem::path relative_to = std::filesystem::relative(to, root);

			//Ensure relative
			if (relative_from.empty() || relative_to.empty() || from.extension() == Assets::descriptor_ext || to.extension() == Assets::descriptor_ext) return;

			//Move file
			if (asset_organizer->moveFile(from, to)) {
			}
		}

		void Manager::removeFile(Assets::GUID const& id) {

			//Get main relative path
			auto it = asset_registry.find(id);
			if (it != asset_registry.end()) {

				//Get main relative path
				auto relative = it->second->main_relative_path;

				//Ensure relative
				if (relative.empty() || relative.extension() == Assets::descriptor_ext) return;

				//Get file path
				std::filesystem::path file_path = services->get<Path::Path>()->resolvePath(Path::main_assets_alias, "");
				file_path /= relative;

				//Check if from is a directory
				if (std::filesystem::is_directory(file_path)) {

					//File operations to delete all
					std::filesystem::remove_all(file_path);
				}
				else {
					//Remove file
					asset_organizer->removeFile(file_path);
				}
			}
		}

		void Manager::removeFile(std::filesystem::path const& file_path) {

			//Get path service
			auto path_service = services->get<Path::Path>();
			std::filesystem::path root = path_service->resolvePath(Path::main_assets_alias + path_service->getVirtualSymbol());
			std::filesystem::path relative = std::filesystem::relative(file_path, root);

			//Ensure relative
			if (relative.empty() || file_path.extension() == Assets::descriptor_ext) return;

			//Check if from is a directory
			if (std::filesystem::is_directory(file_path)) {

				//File operations to delete all
				std::filesystem::remove_all(file_path);
			}
			else {
				//Remove file
				asset_organizer->removeFile(file_path);
			}
		}

		void Manager::duplicateFile(std::filesystem::path const& file_path) {

			//Get root
			auto path_service = services->get<Path::Path>();
			std::filesystem::path root = path_service->resolvePath(Path::main_assets_alias + path_service->getVirtualSymbol());

			//Build new file name
			std::filesystem::path destination = file_path.parent_path() /
				(file_path.stem().string() + " - Copy" + file_path.extension().string());

			//If the duplicate mesh_id, append a number
			int i = 2;
			while (std::filesystem::exists(destination)) {
				destination = file_path.parent_path() /
					(file_path.stem().string() + "- Copy (" + std::to_string(i) + ")" + file_path.extension().string());
				++i;
			}

			//Actually copy the file
			std::filesystem::copy_file(file_path, destination);
		}

		void Manager::saveMaterial(Material const& mat, std::filesystem::path const& out_path) {

			//Get path service
			auto path_service = services->get<Path::Path>();

			//if export succeedds
			if (asset_compiler->ExportMaterial(mat, out_path)) {

				//Get relative
				auto relative = std::filesystem::relative(out_path, path_service->resolvePath(Path::main_assets_alias, ""));

				//Register asset
				if(checkAssetRegistered(relative))registerAsset(relative);
			}
		}
#endif
#endif
	}
}
