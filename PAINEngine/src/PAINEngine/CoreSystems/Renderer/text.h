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
#include "Applications/AppSystem.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "CoreSystems/Renderer/Shader.h"


namespace PAIN {
	class TextRenderer {
	public:
		struct Character {
			unsigned int tex;
			glm::ivec2 size;
			glm::ivec2 bearing;		// offset to top left of glyph
			int advance;    // x offset to next glyph
		};

	private:
		TextRenderer();
		~TextRenderer();

		static bool initialized;
		static std::shared_ptr<Services> services;

		std::unordered_map<unsigned char, Character> characters;

		unsigned int vao, vbo;
		Shader shader;
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
			// this projection gives allows us to work in screen space coordinates
			static const glm::mat4 proj = glm::ortho(0.0f, winWidth * 1.f, 0.0f, winHeight * 1.f);
			return proj;
		}

		void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);

		void debugRenderQuad();
	};
}