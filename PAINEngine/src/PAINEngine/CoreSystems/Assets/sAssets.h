#pragma once

#include "pch.h"
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
#ifdef PN_PLATFORM_WINDOWS
			std::shared_ptr<IAsset> recacheAsset(GUID const& id);
#endif
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

			//Duplicate file function
			void duplicateFile(std::filesystem::path const& file_path);
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
		};

    } // namespace Assets
} // namespace PAIN
