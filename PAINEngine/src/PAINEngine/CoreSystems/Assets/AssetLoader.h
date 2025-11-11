#pragma once

#ifndef ASSET_LOADER_HPP
#define ASSET_LOADER_HPP

#include "Types/Texture.h"
#include "Types/Shader.h"
#include "Types/Text.h"

#include "AssetData.h"

#include "Applications/AppSystem.h"
#include "CoreSystems/Path/Path.h"

#ifdef PN_PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

/*
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace PAIN {
	namespace Assets {

		class RawLoader {
			static unsigned char* _getTextureData(const char* file_path, int& width, int& height, int& num_channels) {
				unsigned char* data = stbi_load(file_path, &width, &height, &num_channels, 0);
				if (data) {
					PN_CORE_INFO("Loaded texture: {} ({}x, {}y, {} channels)\n", file_path, width, height, num_channels);
				}
				else {
					PN_CORE_ERROR("Failed to load texture: {}\n", file_path);
				}
				return data;
			}
		public:
			static unsigned int load(const char* file_path, const std::string& ref) {
				int width, height, num_channels;
				unsigned char* data = _getTextureData(file_path, width, height, num_channels);
				if (!data) {
					return 0;
				}
				unsigned int texture_id;

				glGenTextures(1, &texture_id);
				glBindTexture(GL_TEXTURE_2D, texture_id);

				// tex params
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


				unsigned int internal_format, data_format;
				switch (num_channels) {
				case 3:
					internal_format = GL_RGB8;
					data_format = GL_RGB;
					break;
				case 4:
					internal_format = GL_RGBA8;
					data_format = GL_RGBA;
					break;
				default:
					PN_CORE_ERROR("Unsupported number of channels: {}", num_channels);
					stbi_image_free(data);
					return 0;
				}

				// store texture in vram
				glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, data_format, GL_UNSIGNED_BYTE, data);

				glGenerateMipmap(GL_TEXTURE_2D);

				stbi_image_free(data);
				glBindTexture(GL_TEXTURE_2D, 0);

				texture_map[ref] = texture_id;

				return texture_id;
			}
		};
*/

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
		};
	}
}
#endif
