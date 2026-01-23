#pragma once

#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

#include "Types/Texture.h"
#include "Types/Shader.h"
#include "Types/Text.h"
#include "Types/Scene.h"

#include "AssetData.h"

#include "Applications/AppSystem.h"
#include "CoreSystems/Path/Path.h"

#ifdef PN_PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

namespace PAIN {
	namespace Assets {

		using LoaderFunc = std::function<std::shared_ptr<IAsset>(std::string const&)>;

		class Loader {
		private:

			//Services
			std::shared_ptr<Services> services;
			std::shared_ptr<Path::Path> path_service;

			//Map of loaders
			std::unordered_map<Type, LoaderFunc> asset_loader;

			//Texture helpers
#ifdef PN_PLATFORM_ANDROID
			AAssetManager* asset_manager = nullptr;

		public:
			// Store a pointer to Android's asset manager
			void setAssetManager(AAssetManager* mgr) { asset_manager = mgr; }
		private:

			//Extract ASTC
			void extractKTX(std::string const& virtual_path, std::shared_ptr<Texture> tex) const;
#else
			//Texture data extractor
			void extractDDS(std::string const& virtual_path, std::shared_ptr<Texture> tex) const;
#endif

			// Shader helpers
			uint32_t CompileShader(unsigned int type, const std::string& source) const;
			uint32_t LinkProgram(unsigned int vert_shader, unsigned int frag_shader) const;
			bool CheckShader(GLuint shader, const char* label) const;
			bool CheckProgram(GLuint program) const;

		public:

			Loader(std::shared_ptr<Services> services) : services{ services } {
				path_service = services->get<Path::Path>();
			}
			~Loader() = default;

			//Register loader
			void RegisterLoader(Type const& type, LoaderFunc const& func);

			//Get loader
			LoaderFunc GetLoader(Type const& type) const;

			//Query loader
			bool CheckLoader(Type const& type) const;

			//Import asset registry file
			std::unordered_map<GUID, std::shared_ptr<IAsset>> ImportAssetRegistry(std::string const& virtual_path) const;

			//Importing texture
			std::shared_ptr<Texture> ImportTexture(std::string const& virtual_path) const;

			//Importing model
			std::shared_ptr<Model> ImportModel(std::string const& virtual_path) const;

			//Import shader
			std::shared_ptr<Shader> ImportShader(std::string const& virtual_vert, std::string const& virtual_frag) const;

			//Import Font
			std::shared_ptr<Fonts::FontFace> ImportFont(std::string const& virtual_path) const;

			//Import material
			std::shared_ptr<Material> ImportMaterial(std::string const& virtual_path) const;

			//Import scene
			std::shared_ptr<Scene::SceneAsset> ImportScene(std::string const& virtual_path) const;\
		};
	}
}
#endif
