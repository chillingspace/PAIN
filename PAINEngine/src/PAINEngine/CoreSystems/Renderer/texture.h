/*****************************************************************//**
 * \file   texture.h
 * \brief  
 * 
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/


#include "pch.h"


namespace PAIN {
	class TextureManager {
	private:
		TextureManager() = default;
		~TextureManager();

	private:
		std::unordered_map<std::string, unsigned int> texture_map;

		static unsigned char* _getTextureData(const char* file_path, int& width, int& height, int& num_channels);

	public:
		static TextureManager& get() {
			static TextureManager instance;
			return instance;
		}

		unsigned int load(const char* file_path, const std::string& ref);
	};
}