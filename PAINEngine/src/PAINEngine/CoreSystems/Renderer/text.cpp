/*****************************************************************//**
 * \file   text.cpp
 * \brief
 *
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/


#include "pch.h"
#include "text.h"

#ifdef PN_PLATFORM_ANDROID
#include "Utility/AndroidFs.h"
#endif

bool PAIN::TextRenderer::initialized = false;
std::shared_ptr<PAIN::Services> PAIN::TextRenderer::services = nullptr;

namespace PAIN {
	TextRenderer::TextRenderer() {

		// compile and link shader
		{
#ifdef PN_PLATFORM_ANDROID
			std::filesystem::path text_path = "engine\\shaders\\android_text.vert";
#else
			std::filesystem::path text_path = "engine\\shaders\\text.vert";
#endif

			shader = services->get<Assets::Manager>()->getAsset<Assets::Shader>(text_path);
			PN_CORE_INFO("Text shader compiled, ID: {}", shader->GetRendererID());
		}
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error after compiling text shader: {}", error);
		}

		// creating vao/vbo for rendering text quads
		{
			glGenVertexArrays(1, &vao);
			glGenBuffers(1, &vbo);
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		GLenum err = glGetError();
		while (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error in TextRenderer constructor: {}", err);
			err = glGetError();
		}
	}

	TextRenderer::~TextRenderer() {
		// cleanup vao/vbo
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);

	}

	void TextRenderer::renderText(std::shared_ptr<Assets::Fonts::FontGlyphAtlas> font, const std::string& text, float x, float y, float scale, const glm::vec3& color)
	{
		if (shader->GetRendererID() == 0) {
			PN_CORE_ERROR("TextRenderer shader not initialized!");
			return;
		}

		// activate corresponding render state	
		shader->Bind();

		glActiveTexture(GL_TEXTURE0);
		shader->SetUniform("projection", projection());
		shader->SetUniform("textColor", color);
		shader->SetUniform("text", 0);
		glBindVertexArray(vao);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// iterate through all characters
		std::string::const_iterator c;
		for (c = text.begin(); c != text.end(); c++)
		{
			Assets::Fonts::Character ch = font->glyphs[*c];

			float xpos = x + ch.bearing.x * scale;
			float ypos = y - (ch.size.y - ch.bearing.y) * scale;

			float w = ch.size.x * scale;
			float h = ch.size.y * scale;
			// update VBO for each character
			float vertices[6][4] = {
				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos,     ypos,       0.0f, 1.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },

				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },
				{ xpos + w, ypos + h,   1.0f, 0.0f }
			};
			// render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch.tex);
			// update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
			x += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
		}

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_BLEND);
	}

	void TextRenderer::debugRenderQuad() {
	}

}