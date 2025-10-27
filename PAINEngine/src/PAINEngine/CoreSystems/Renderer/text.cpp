/*****************************************************************//**
 * \file   text.cpp
 * \brief
 *
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/


#include "pch.h"
#include "text.h"

bool PAIN::TextRenderer::initialized = false;
std::shared_ptr<PAIN::Services> PAIN::TextRenderer::services = nullptr;

namespace PAIN {
	TextRenderer::TextRenderer() {
		FT_Library library;
		FT_Face face;
		const std::string path = services->get<Path::Path>()->resolvePath("engine_assets://fonts/OpenSans-Regular.ttf");

		// Initialize FreeType library
		if (FT_Init_FreeType(&library)) {
			PN_CORE_ERROR("Could not initialize FreeType library");
		}

		// Load font file
		if (FT_New_Face(library, path.c_str(), 0, &face)) {
			PN_CORE_ERROR("Could not load font");
		}

		// Set font size (width=0 means auto-calculate, height=16*64)
		FT_Set_Pixel_Sizes(face, 0, 48);

		// Load a character glyph (e.g., 'A')
		if (FT_Load_Char(face, 'A', FT_LOAD_RENDER)) {
			PN_CORE_ERROR("Could not load character");
		}

		// Access the glyph bitmap
		FT_GlyphSlot slot = face->glyph;
		FT_Bitmap bitmap = slot->bitmap;

		// Print some glyph information
		PN_CORE_INFO("Glyph width: {}", bitmap.width);
		PN_CORE_INFO("Glyph height: {}", bitmap.rows);
		PN_CORE_INFO("Bearing X: {}", slot->bitmap_left);
		PN_CORE_INFO("Bearing Y: {}", slot->bitmap_top);
		PN_CORE_INFO("Advance: {}", slot->advance.x / 64);

		// Simple ASCII art rendering to console
		for (unsigned int y = 0; y < bitmap.rows; y++) {
			for (unsigned int x = 0; x < bitmap.width; x++) {
				unsigned char pixel = bitmap.buffer[y * bitmap.width + x];
				// Note: This would need your own output method
				// std::cout << (pixel > 128 ? '#' : ' ');
			}
		}

		FT_Done_Face(face);
		FT_Done_FreeType(library);
	}

	TextRenderer::~TextRenderer() {
	}

}