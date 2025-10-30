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
int PAIN::TextRenderer::winWidth = 0;
int PAIN::TextRenderer::winHeight = 0;
std::shared_ptr<PAIN::Services> PAIN::TextRenderer::services = nullptr;

namespace PAIN {
	TextRenderer::TextRenderer() {
		// initial loading of font glyphs into vram
		{
			if (!glGetString(GL_VERSION)) {
				PN_CORE_ERROR("No OpenGL context when creating TextRenderer!");
				return;
			}
			else {
				PN_CORE_INFO("TextRenderer ctor OpenGL context ok");
			}

			FT_Library library;
			FT_Face face;
			const std::string path = services->get<Path::Path>()->resolvePath("engine_assets://fonts/OpenSans-Regular.ttf");

			if (FT_Init_FreeType(&library)) {
				PN_CORE_ERROR("Could not initialize FreeType library");
			}

#ifdef PN_PLATFORM_ANDROID
			// On Android, read the font data from assets into memory
			std::string fontData = ReadFileAndroid(path);
			if (fontData.empty()) {
				PN_CORE_ERROR("Could not read font file from Android assets");
				return;
			}

			// Load font from memory buffer
			if (FT_New_Memory_Face(library,
				reinterpret_cast<const FT_Byte*>(fontData.data()),
				fontData.size(),
				0,
				&face)) {
				PN_CORE_ERROR("Could not load font from memory");
				return;
			}

			// store in member var so that it stays in memory
			fontDataBuffer = std::move(fontData);

#else
			if (FT_New_Face(library, path.c_str(), 0, &face)) {
				PN_CORE_ERROR("Could not load font");
			}
#endif

			// Set font size (width=0 means auto-calculate)
			FT_Set_Pixel_Sizes(face, 0, 48);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
			// load characters
			for (unsigned char c{}; c < 128; c++) {
				if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
					PN_CORE_ERROR("Could not load character {}", c);
				}

				unsigned int char_tex;
				glGenTextures(1, &char_tex);
				glBindTexture(GL_TEXTURE_2D, char_tex);
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_RED,
					face->glyph->bitmap.width,
					face->glyph->bitmap.rows,
					0,
					GL_RED,
					GL_UNSIGNED_BYTE,
					face->glyph->bitmap.buffer
				);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				Character character = {
					char_tex,
					{ face->glyph->bitmap.width, face->glyph->bitmap.rows },
					{ face->glyph->bitmap_left, face->glyph->bitmap_top },
					static_cast<int>(face->glyph->advance.x)
				};
				characters[c] = character;
			}
			glBindTexture(GL_TEXTURE_2D, 0);

			FT_Done_Face(face);
			FT_Done_FreeType(library);
		}

		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error before compiling text shader 1: {}", error);
		}


		// compile and link shader
		{
#ifdef PN_PLATFORM_ANDROID
			std::string path_vert = "engine_assets://shaders/android_text.vert";
			std::string path_frag = "engine_assets://shaders/android_text.frag";

			const std::string vert = ReadFileAndroid(services->get<Path::Path>()->resolvePath(path_vert));
			const std::string frag = ReadFileAndroid(services->get<Path::Path>()->resolvePath(path_frag));
#else
			std::string path_vert = "engine_assets://shaders/text.vert";
			std::string path_frag = "engine_assets://shaders/text.frag";


			std::ifstream ifs(services->get<Path::Path>()->resolvePath(path_vert));
			std::stringstream buffer;
			buffer << ifs.rdbuf();
			const std::string vert = buffer.str();

			ifs.close();

			ifs.open(services->get<Path::Path>()->resolvePath(path_frag));
			buffer.str(std::string());
			buffer << ifs.rdbuf();
			const std::string frag = buffer.str();
#endif

			error = glGetError();
			if (error != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL error before compiling text shader 2: {}", error);
			}


			PN_CORE_INFO("Compiling text shader from {} and {}", path_vert, path_frag);
			PN_CORE_TRACE("Vertex shader source:\n{0}", vert);
			shader = Shader(vert.c_str(), frag.c_str());
			PN_CORE_INFO("Text shader compiled, ID: {}", shader.GetRendererID());
		}
		error = glGetError();
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
		// cleanup textures
		for (auto& pair : characters) {
			glDeleteTextures(1, &pair.second.tex);
		}
		// cleanup vao/vbo
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);

	}

	void TextRenderer::renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color)
	{
		if (shader.GetRendererID() == 0) {
			PN_CORE_ERROR("TextRenderer shader not initialized!");
			return;
		}

		// activate corresponding render state	
		shader.Bind();

		glActiveTexture(GL_TEXTURE0);
		shader.SetUniform("projection", projection());
		shader.SetUniform("textColor", color);
		shader.SetUniform("text", 0);
		glBindVertexArray(vao);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// iterate through all characters
		std::string::const_iterator c;
		for (c = text.begin(); c != text.end(); c++)
		{
			Character ch = characters[*c];

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