#pragma once

#ifndef ASSETS_SERVICE_HPP
#define ASSETS_SERVICE_HPP

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

			//Asset dependencies
			std::unordered_map<GUID, std::unordered_set<GUID>> dependencies;

			//Asset loader
			std::unique_ptr<Loader> asset_loader;

			//Use shared_mutex for read-write locking (better performance)
			mutable std::shared_mutex registry_mutex;  // Protects asset_registry + path maps
			mutable std::shared_mutex cache_mutex;      // Protects asset_cache

			//Separate mutex for in-flight loading
			mutable std::mutex loading_mutex;           // Protects loading_assets
			std::unordered_set<GUID> loading_assets;    // Track which assets are currently 

#ifdef PN_PLATFORM_WINDOWS
			//Asset Organizer
			std::unique_ptr<Organizer> asset_organizer;
			
			//Asset compiler
			std::unique_ptr<Compiler> asset_compiler;
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
			void unregisterAsset(std::filesystem::path const& relative_path);
			void unregisterAsset(GUID const& id);

			//Check asset registered
			bool checkAssetRegistered(std::filesystem::path const& relative_path);
			bool checkAssetRegistered(GUID const& id) const;

			//Get asset
			template <typename T>
			std::optional<std::shared_ptr<T>> getAsset(std::filesystem::path const& relative_path) {

				//Check if path is empty early return
				if(relative_path.empty()) return std::nullopt;

				//Find GUID
				auto id = findGUID(relative_path);

				//Check GUID
				if (!id.IsValid() || !checkAssetRegistered(id)) return std::nullopt;

				//Get asset
				return getAsset<T>(id);
			}
			
			template <typename T>
			std::optional<std::shared_ptr<T>> getAsset(GUID const& id) {

				// Validate GUID
				if (!id.IsValid()) return std::nullopt;

				//PHASE 1: Check registration (read lock)
				bool is_cacheable;
				{
					std::shared_lock<std::shared_mutex> registry_lock(registry_mutex);
					auto registry_it = asset_registry.find(id);
					if (registry_it == asset_registry.end()) {
						return std::nullopt;
					}
					is_cacheable = Assets::isAssetCacheable(registry_it->second->type);
				}

				std::shared_ptr<IAsset> asset;

				//PHASE 2: Get or load asset
				if (is_cacheable) {
					// Check cache first
					{
						std::shared_lock<std::shared_mutex> cache_lock(cache_mutex);
						auto cache_it = asset_cache.find(id);
						if (cache_it != asset_cache.end()) {
							asset = cache_it->second;
						}
					}

					// Not cached - load it
					if (!asset) {
						asset = cacheAsset(id);
					}
				}
				else {
					// Non-cacheable - load directly without caching
					std::shared_ptr<IAsset> asset_data;
					{
						std::shared_lock<std::shared_mutex> registry_lock(registry_mutex);
						asset_data = asset_registry[id];
					}

					auto virtual_path = services->get<Path::Path>()->aliasCombineRelative(
						Path::assets_alias,
						asset_data->shipped_relative_path.string()
					);

					//Load without locks
					asset = asset_loader->GetLoader(asset_data->type)(virtual_path);

					// Copy metadata
					asset->guid = asset_data->guid;
					asset->main_relative_path = asset_data->main_relative_path;
					asset->shipped_relative_path = asset_data->shipped_relative_path;
					asset->name = asset_data->name;
					asset->type = asset_data->type;
				}

				//PHASE 3: Handle texture upload (if needed)
				if (asset->type == Assets::Type::Texture) {
					auto tex = std::dynamic_pointer_cast<Texture>(asset);
					if (tex && !tex->gl_texture) {
						asset_loader->uploadTexture(tex);
					}
				}

				//PHASE 4: Cast and return
				auto typed_asset = std::dynamic_pointer_cast<T>(asset);
				if (!typed_asset) {
					throw std::runtime_error("Asset type mismatch");
				}

				return typed_asset;
			}

			//Get all assets of type
			template<typename T>
			std::vector<std::shared_ptr<T>> getAllAssetsOfType(Type const& type) {

				//Declare temp container
				std::vector<std::shared_ptr<T>> container;

				//Find all assets with type
				std::shared_lock<std::shared_mutex> registry_lock(registry_mutex);
				for (auto const& asset : asset_registry) {
					if (asset.second->type == type) {
						auto opt_asset = getAsset<T>(asset.first);
						if(opt_asset.has_value()) container.push_back(opt_asset.value());
					}
				}

				//Return all assets
				return container;
			}

			//Caching of assets
			std::shared_ptr<IAsset> cacheAsset(GUID const& id);
			void batchCacheAssets(std::unordered_set<GUID> batch_ids);
			void batchUploadAllCachedTextures();
			void uncacheAsset(GUID const& id);
			void clearAssetCache();
#ifdef PN_PLATFORM_WINDOWS
			std::shared_ptr<IAsset> recacheAsset(GUID const& id);
			void reshipAsset(GUID const& id);
#endif
			bool checkAssetCached(GUID const& id) const;

			//Find asset type
			std::shared_ptr<IAsset> getAssetData(GUID const& id) const;
			std::shared_ptr<IAsset> getAssetData(std::filesystem::path const& relative_path);
			std::vector <std::shared_ptr<IAsset>> getAllAssetDataOfType(Type const& type);

#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG

			//Debug only editor mode functions

			//Move file function
			void moveFile(std::filesystem::path const& from, std::filesystem::path const& to);

			//Delete fil overload
			void removeFile(Assets::GUID const& id);

			//Delete file function
			void removeFile(std::filesystem::path const& file_path);

			//Duplicate file function
			void duplicateFile(std::filesystem::path const& file_path);

			//Create new material function
			void saveMaterial(Material const& mat, std::filesystem::path const& out_path);
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

			GUID findByName(const std::string& name); // wraps findGUID
			Type getTypeByGUID(const GUID& id) const; // wraps getAssetData(id)
			Loader* getRawAssetLoader();
		};

    } // namespace Assets
} // namespace PAIN

#endif
