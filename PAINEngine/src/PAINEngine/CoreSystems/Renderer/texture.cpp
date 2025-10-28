/*****************************************************************//**
 * \file   texture.cpp
 * \brief
 *
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/


#include "pch.h"
#include "texture.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"

#ifdef PN_PLATFORM_ANDROID
#include "Utility/AndroidFs.h"
#endif



namespace PAIN {
	TextureManager::TextureManager() {
		stbi_set_flip_vertically_on_load(true);
	}

	TextureManager::~TextureManager() {
		for (const auto& [ref, tex_id] : texture_map) {
			glDeleteTextures(1, &tex_id);
			PN_CORE_INFO("Deleted texture: {} (ID: {})", ref, tex_id);
		}
		texture_map.clear();
	}


	unsigned char* TextureManager::_getTextureData(const char* file_path, int& width, int& height, int& num_channels) {
#ifdef PN_PLATFORM_WINDOWS
		unsigned char* data = stbi_load(file_path, &width, &height, &num_channels, 0);
		if (data) {
			PN_CORE_INFO("Loaded texture: {} ({}x, {}y, {} channels)\n", file_path, width, height, num_channels);
		}
		else {
			PN_CORE_ERROR("Failed to load texture: {}\n", file_path);
		}
		return data;
#else
		std::string fileData = ReadFileAndroid(file_path);

		if (fileData.empty()) {
			PN_CORE_ERROR("Failed to read file: {}", file_path);
			return nullptr;
		}

		// Load from memory buffer
		unsigned char* data = stbi_load_from_memory(
			reinterpret_cast<const unsigned char*>(fileData.data()),
			fileData.size(),
			&width,
			&height,
			&num_channels,
			0
		);

		if (data) {
			PN_CORE_INFO("Loaded texture: {} ({}x{}, {} channels)", file_path, width, height, num_channels);
			return data;
		}
		PN_CORE_ERROR("Failed to decode texture: {} - Reason: {}", file_path, stbi_failure_reason());
#endif
	}

	unsigned int TextureManager::load(const char* file_path, const std::string& ref) {
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
		case 1:
			// for single channel textures like roughness, metallic, ao etc.
			internal_format = GL_R8;
			data_format = GL_RED;
			break;
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
}
