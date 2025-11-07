#pragma once

#ifndef ASSETS_TEXT_HPP
#define ASSETS_TEXT_HPP

#include "AssetTypes.h"

#include "CoreSystems/Path/Path.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace PAIN {
	namespace Assets {
		namespace Fonts {


			//Default pixel size
			const int DEF_PIXEL_SIZE = 48;

			struct Character {
				unsigned int tex;
				glm::ivec2 size;
				glm::ivec2 bearing;		// offset to top left of glyph
				int advance;    // x offset to next glyph
			};

			struct FontGlyphAtlas {
				std::unordered_map<unsigned char, Character> glyphs;
				int pixelSize = 0;
			};

			class FontFace : public PAIN::Assets::IAsset {
			private:
				//Path service
				std::shared_ptr<Path::Path> path_service;

				//Virtual path
				std::string virtual_path;

				//Android font buffer
				std::string font_buffer;

				//Atlas pixel cache
				std::unordered_map<int, std::shared_ptr<FontGlyphAtlas>> pixel_to_atlas;

				//Internal function to cache pixel
				void cachePixel(int pixel_size);
			public:
				//Construct font face
				FontFace(std::shared_ptr<Path::Path> path_service, std::string const& virtual_path) : path_service{ path_service }, virtual_path{ virtual_path } {
					//Cache a default pixel
					cachePixel(DEF_PIXEL_SIZE);
				}

				//Destructor
				~FontFace() override;

				//Get atlas
				std::shared_ptr<FontGlyphAtlas> getFont(int pixel_size = DEF_PIXEL_SIZE);
			};
		}
	}
}

#endif
