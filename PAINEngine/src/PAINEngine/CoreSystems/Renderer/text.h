/*****************************************************************//**
 * \file   text.h
 * \brief
 *
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/

#pragma once

#include "pch.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Assets/sAssets.h"
#include "Applications/AppSystem.h"
#include <ft2build.h>
#include FT_FREETYPE_H

#include "CoreSystems/Windows/Window.h"

namespace PAIN {
	class TextRenderer {
	public:

	private:
		TextRenderer();
		~TextRenderer();

		static bool initialized;
		static std::shared_ptr<Services> services;

		unsigned int vao, vbo;
		std::shared_ptr<Assets::Shader> shader;

		float measureTextWidth(const std::string& text, std::shared_ptr<Assets::Fonts::FontGlyphAtlas> font, float scale);
		std::vector<std::string> handleTextWrap(const PAIN::UIText& text_comp);
		void renderTextShadow(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor);
		void renderTextOutline(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor);
		void renderGlyph(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor);
	public:
		// dependency injection
		static void init(std::shared_ptr<Services> s) {
			services = s;
			initialized = true;
		}

		static TextRenderer& get() {
			if (!initialized) {
                throw std::runtime_error("TextRenderer not initialized!");
			}
			static TextRenderer instance;
			return instance;
		}

		glm::mat4 projection() {
			auto win_dim = services->get<Window::Window>()->getFrameBuffer();
			// this projection gives allows us to work in screen space coordinates
			static const glm::mat4 proj = glm::ortho(0.0f, win_dim.x * 1.f, 0.0f, win_dim.y * 1.f);
			return proj;
		}

		void renderText(UIText& text_comp);

		void debugRenderQuad();
	};
}