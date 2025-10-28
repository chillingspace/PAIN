#pragma once

#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

#include "AssetTypes.h"
#include "AssetData.h"

namespace PAIN {
	namespace Assets {

		using LoaderFunc = std::function<std::shared_ptr<IAsset>(std::filesystem::path const&)>;

		class Loader {
		private:

			std::unordered_map<Type, LoaderFunc> asset_loader;

			//Texture data extractor
			void extractDDS(std::filesystem::path const& path, std::shared_ptr<Texture> tex) const;
			void extractASTC(std::filesystem::path const& path, std::shared_ptr<Texture> tex) const;

		public:

			Loader() {

				//Register texture loader
				asset_loader[Type::Texture] = [this](std::filesystem::path const& primary_path) {
					return ImportTexture(primary_path.string());
				};

				//Register model loader
				asset_loader[Type::Model] = [this](std::filesystem::path const& primary_path) {
					return ImportModel(primary_path.string());
					};
			}
			~Loader() = default;

			//Get loader
			LoaderFunc GetLoader(Type const& type) const;

			//Query loader
			bool CheckLoader(Type const& type) const;

			//Import asset registry file
			std::unordered_map<GUID, IAsset> ImportAssetRegistry(std::filesystem::path const& path) const;

			//Importing texture
			std::shared_ptr<Texture> ImportTexture(std::filesystem::path const& path) const;

			//Importing model
			std::shared_ptr<Model> ImportModel(std::filesystem::path const& path) const;
		};
	}
}
#endif
