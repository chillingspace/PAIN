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

		void renderText(std::shared_ptr<Assets::Fonts::FontGlyphAtlas> font, const std::string& text, float x, float y, float scale, const glm::vec3& color);

		void debugRenderQuad();
	};
}