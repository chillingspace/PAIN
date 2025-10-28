#pragma once

#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

#include "AssetTypes.h"
#include "AssetData.h"

namespace PAIN {
	namespace Assets {

		class Loader {
		private:
			//Texture data extractor
			void extractDDS(std::filesystem::path const& path, std::shared_ptr<Texture> tex);
			void extractASTC(std::filesystem::path const& path, std::shared_ptr<Texture> tex);


		public:

			Loader() = default;
			~Loader() = default;

			//Importing texture
			std::shared_ptr<Texture> ImportTexture(std::filesystem::path const& path);

			//Importing model
			std::shared_ptr<Model> ImportModel(std::filesystem::path const& path);
		};
	}
}
#endif
