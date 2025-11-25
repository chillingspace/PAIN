/*****************************************************************//**
 * \file   text.cpp
 * \brief
 *
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/


#include "pch.h"
#include "text.h"
#include "ECS/Components/cUIComps.h"

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

			auto shader_opt = services->get<Assets::Manager>()->getAsset<Assets::Shader>(text_path);
			shader = shader_opt.has_value() ? shader_opt.value() : shader;
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

	float TextRenderer::measureTextWidth(const std::string& text, std::shared_ptr<Assets::Fonts::FontGlyphAtlas> font, float scale)
	{
		float width = 0.0f;
		for (char c : text) {
			const auto& ch = font->glyphs[c];
			width += (ch.advance >> 6) * scale;
		}
		return width;
	}

    std::vector<std::string> TextRenderer::handleTextWrap(const PAIN::UIText& text_comp)
    {
        std::vector<std::string> lines;
        std::string curr_line, curr_word;
        float curr_width = 0.f;
        float word_width = 0.f;
        float wrap_limit = text_comp.word_wrap ? text_comp.max_length : FLT_MAX;
        int length = (text_comp.max_length > 0) ? std::min<int>(text_comp.max_length, text_comp.display_text.length()) : text_comp.display_text.length();

        for (int i = 0; i < length; ++i) {
            char ch = text_comp.display_text[i];
            if (ch == ' ' || ch == '\n' || i == length - 1) {
                if (i == length - 1 && ch != '\n' && ch != ' ') curr_word.push_back(ch);

                auto font_opt = services->get<Assets::Manager>()->getAsset<Assets::Fonts::FontFace>(text_comp.font_guid);

                if (!font_opt) { continue; }

                auto font = font_opt.value().get()->getFont();

                if (!font) { continue; }

                word_width = measureTextWidth(curr_word, font, text_comp.font_size);

                if (curr_width + word_width > wrap_limit && curr_line.length() > 0) {
                    lines.push_back(curr_line);
                    curr_line = "";
                    curr_width = 0.f;
                }
                curr_line += curr_word;
                curr_width += word_width;

                if (ch == ' ' || ch == '\n') {
                    curr_line.push_back(ch);
                    curr_width += measureTextWidth(std::string(1, ch), font, text_comp.font_size);
                }
                if (ch == '\n') {
                    lines.push_back(curr_line);
                    curr_line = "";
                    curr_width = 0.f;
                }
                curr_word = "";
            }
            else {
                curr_word.push_back(ch);
            }
        }
        if (!curr_line.empty()) lines.push_back(curr_line);

        return lines;
    }

    void TextRenderer::renderTextShadow(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor)
    {
        if (text_comp.shadow_color.a == 0.f || (text_comp.shadow_offset.x == 0.f && text_comp.shadow_offset.y == 0.f))
            return;

        float font_size = text_comp.font_size;
        float x_pos = x_cursor + ch.bearing.x * font_size;
        float y_pos = y_cursor - (ch.size.y - ch.bearing.y) * font_size;
        float width = ch.size.x * font_size;
        float height = ch.size.y * font_size;

        float shadow_x = x_pos + text_comp.shadow_offset.x;
        float shadow_y = y_pos + text_comp.shadow_offset.y;

        shader->SetUniform("textColor", glm::vec3(text_comp.shadow_color));
        float vertices[6][4] = {
            { shadow_x, shadow_y + height, 0.0f, 0.0f },
            { shadow_x, shadow_y, 0.0f, 1.0f },
            { shadow_x + width, shadow_y, 1.0f, 1.0f },
            { shadow_x, shadow_y + height, 0.0f, 0.0f },
            { shadow_x + width, shadow_y, 1.0f, 1.0f },
            { shadow_x + width, shadow_y + height, 1.0f, 0.0f }
        };
        glBindTexture(GL_TEXTURE_2D, ch.tex);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void TextRenderer::renderTextOutline(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor)
    {
        if (text_comp.outline_thickness <= 0.0f || text_comp.outline_color.a == 0.f)
            return;

        float font_size = text_comp.font_size;
        float x_pos = x_cursor + ch.bearing.x * font_size;
        float y_pos = y_cursor - (ch.size.y - ch.bearing.y) * font_size;
        float width = ch.size.x * font_size;
        float height = ch.size.y * font_size;

        shader->SetUniform("textColor", glm::vec3(text_comp.outline_color));
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy)
            {
                if (dx == 0 && dy == 0) continue; // skip center
                float o_x = x_pos + dx * text_comp.outline_thickness;
                float o_y = y_pos + dy * text_comp.outline_thickness;
                float vertices[6][4] = {
                    { o_x, o_y + height, 0.0f, 0.0f },
                    { o_x, o_y, 0.0f, 1.0f },
                    { o_x + width, o_y, 1.0f, 1.0f },
                    { o_x, o_y + height, 0.0f, 0.0f },
                    { o_x + width, o_y, 1.0f, 1.0f },
                    { o_x + width, o_y + height, 1.0f, 0.0f }
                };
                glBindTexture(GL_TEXTURE_2D, ch.tex);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
    }

    void TextRenderer::renderGlyph(const Assets::Fonts::Character& ch, const UIText& text_comp, float x_cursor, float y_cursor)
    {
        float x_pos = x_cursor + ch.bearing.x * text_comp.font_size;
        float y_pos = y_cursor - (ch.size.y - ch.bearing.y) * text_comp.font_size;
        float width = ch.size.x * text_comp.font_size;
        float height = ch.size.y * text_comp.font_size;

        shader->SetUniform("textColor", text_comp.color);
        float vertices[6][4] = {
            { x_pos, y_pos + height, 0.0f, 0.0f },
            { x_pos, y_pos, 0.0f, 1.0f },
            { x_pos + width, y_pos, 1.0f, 1.0f },
            { x_pos, y_pos + height, 0.0f, 0.0f },
            { x_pos + width, y_pos, 1.0f, 1.0f },
            { x_pos + width, y_pos + height, 1.0f, 0.0f }
        };
        glBindTexture(GL_TEXTURE_2D, ch.tex);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void TextRenderer::renderText(UIText& text_comp)
    {
        if (shader->GetRendererID() == 0) {
            PN_CORE_ERROR("TextRenderer shader not initialized!");
            return;
        }

        // Get font asset
        auto font_opt = services->get<Assets::Manager>()->getAsset<Assets::Fonts::FontFace>(text_comp.font_guid);
        if (!font_opt.has_value()) return;
        auto font = font_opt.value().get()->getFont();
        if (!font) return;

        shader->Bind();
        glActiveTexture(GL_TEXTURE0);
        shader->SetUniform("projection", projection());
        glBindVertexArray(vao);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // --- 1. Generate wrapped lines ---
        std::vector<std::string> lines = handleTextWrap(text_comp);

        float y_cursor = text_comp.text_pos.y;

        for (const auto& line : lines) {
            // --- 2. Alignment (center/right/left) ---
            float line_width = measureTextWidth(line, font, text_comp.font_size);
            float x_aligned = text_comp.text_pos.x;
            if (text_comp.alignment == TextAlignment::Center)
                x_aligned -= 0.5f * line_width;
            else if (text_comp.alignment == TextAlignment::Left)
                x_aligned -= line_width;

            float x_cursor = x_aligned;
            for (size_t i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (font->glyphs.count(c) == 0) continue;
                const auto& ch = font->glyphs[c];

                // Text shadow
                renderTextShadow(ch, text_comp, x_cursor, y_cursor);

                // Text outline
                renderTextOutline(ch, text_comp, x_cursor, y_cursor);

                // Main glyph 
                renderGlyph(ch, text_comp, x_cursor, y_cursor);

                x_cursor += (ch.advance >> 6) * text_comp.font_size;
            }
            y_cursor += text_comp.font_size * text_comp.line_height;
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
    }



	void TextRenderer::debugRenderQuad() {
	}

}