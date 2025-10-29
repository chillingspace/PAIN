#pragma once

#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

#include "AssetTypes.h"
#include "AssetData.h"

#ifdef PN_PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

namespace PAIN {
	namespace Assets {

		using LoaderFunc = std::function<std::shared_ptr<IAsset>(std::string const&)>;

		class Loader {
		private:

			std::unordered_map<Type, LoaderFunc> asset_loader;

#ifdef PN_PLATFORM_ANDROID
			AAssetManager* asset_manager = nullptr;

		public:
			// Store a pointer to Android's asset manager
			void setAssetManager(AAssetManager* mgr) { asset_manager = mgr; }
		private:

			//Extract ASTC
			void extractASTC(std::vector<uint8_t> const& data, std::shared_ptr<Texture> tex) const;
#else
			//Texture data extractor
			void extractDDS(std::vector<uint8_t> const& data, std::shared_ptr<Texture> tex) const;
#endif

		public:

			Loader() = default;
			~Loader() = default;

			//Register loader
			void RegisterLoader(Type const& type, LoaderFunc const& func);

			//Get loader
			LoaderFunc GetLoader(Type const& type) const;

			//Query loader
			bool CheckLoader(Type const& type) const;

			//Import asset registry file
			std::unordered_map<GUID, IAsset> ImportAssetRegistry(nlohmann::json const& json_package) const;

			//Importing texture
			std::shared_ptr<Texture> ImportTexture(std::vector<uint8_t> const& data) const;

			//Importing model
			std::shared_ptr<Model> ImportModel(std::vector<uint8_t> const& data) const;
		};
	}
}
#endif
