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
		static std::weak_ptr<Services> services;

		unsigned int vao, vbo;
		std::shared_ptr<Assets::Shader> shader;

		float measureTextWidth(const std::string& text, std::shared_ptr<Assets::Fonts::FontGlyphAtlas> font, float scale);
		std::vector<std::string> handleTextWrap(const PAIN::UIText& text_comp);
		void renderTextShadow(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor, float font_final_scale);
		void renderTextOutline(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor, float font_final_scale);
		void renderGlyph(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor, float font_final_scale);
	public:
		// dependency injection
		static void init(std::shared_ptr<Services> s) {
			services = s;
			initialized = true;
		}

		static void shutdown() {
			services.reset();
			initialized = false;
		}

		static TextRenderer& get() {
			if (!initialized) {
                throw std::runtime_error("TextRenderer not initialized!");
			}
			static TextRenderer instance;
			return instance;
		}

		glm::mat4 projection() {
			auto svc = services.lock();
			if (!svc) {
				throw std::runtime_error("TextRenderer services expired");
			}
			auto win_dim = svc->get<Window::Window>()->getFrameBuffer();
			// this projection gives allows us to work in screen space coordinates
			// Recalculate every call to handle window resize
			const glm::mat4 proj = glm::ortho(0.0f, (float)win_dim.x, 0.0f, (float)win_dim.y);
			return proj;
		}

		void renderText(UIText& text_comp);

		void debugRenderQuad();
	};
}
