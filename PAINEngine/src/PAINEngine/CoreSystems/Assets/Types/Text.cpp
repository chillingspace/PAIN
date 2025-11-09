#include "pch.h"
#include "Text.h"

namespace PAIN {
	namespace Assets {
        namespace Fonts {

            void FontFace::cachePixel(int pixel_size) {

                //Check if pixel_size has been already cached
                if (pixel_to_atlas.find(pixel_size) != pixel_to_atlas.end()) {
                    return;
                }

                if (!glGetString(GL_VERSION)) {
                    PN_CORE_ERROR("No OpenGL context when creating TextRenderer!");
                    return;
                }
                else {
                    PN_CORE_INFO("TextRenderer ctor OpenGL context ok");
                }

                FT_Library library;
                FT_Face face = nullptr;

                if (FT_Init_FreeType(&library)) {
                    PN_CORE_ERROR("Could not initialize FreeType library");
                    FT_Done_Face(face);
                    FT_Done_FreeType(library);
                    return;
                }

#ifdef PN_PLATFORM_ANDROID

                //Check if font buffer has not been read, init buffer
                if (font_buffer.empty()) {
                    auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
                    std::string fontData(stream->size(), '\0');
                    stream->read(fontData.data(), fontData.size());
                    if (fontData.empty()) {
                        PN_CORE_ERROR("Could not read font file from Android assets");
                        FT_Done_Face(face);
                        FT_Done_FreeType(library);
                        return;
                    }

                    // store in member var so that it stays in memory
                    font_buffer = std::move(fontData);
                }

                // Load font from memory buffer
                if (FT_New_Memory_Face(library,
                    reinterpret_cast<const FT_Byte*>(font_buffer.data()),
                    font_buffer.size(),
                    0,
                    &face)) {
                    PN_CORE_ERROR("Could not load font from memory");
                    FT_Done_Face(face);
                    FT_Done_FreeType(library);
                    return;
                }
#else
                const std::string path = path_service->resolvePath(virtual_path);
                if (FT_New_Face(library, path.c_str(), 0, &face)) {
                    PN_CORE_ERROR("Could not load font");
                    FT_Done_Face(face);
                    FT_Done_FreeType(library);
                    return;
                }
#endif
                //Create temp atlas
                std::shared_ptr<FontGlyphAtlas> atlas = std::make_shared<FontGlyphAtlas>();
                atlas->pixelSize = pixel_size;

                // Set font size (width=0 means auto-calculate)
                FT_Set_Pixel_Sizes(face, 0, atlas->pixelSize);

                glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction
                // load characters
                for (unsigned char c{}; c < 128; c++) {
                    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                        PN_CORE_ERROR("Could not load character {}", c);
                        continue;
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
                    atlas->glyphs[c] = character;
                }
                glBindTexture(GL_TEXTURE_2D, 0);

                FT_Done_Face(face);
                FT_Done_FreeType(library);

                //Input into pixel to atlas
                pixel_to_atlas[pixel_size] = atlas;

                GLenum error = glGetError();
                if (error != GL_NO_ERROR) {
                    PN_CORE_ERROR("OpenGL error in loading font atlas: {}", error);
                }
            }

            FontFace::~FontFace() {

                //cleanup textures
                for (auto& atlas : pixel_to_atlas) {
                    for (auto& glyphs : atlas.second->glyphs)
                        glDeleteTextures(1, &glyphs.second.tex);
                }
            }

            std::shared_ptr<FontGlyphAtlas> FontFace::getFont(int pixel_size) {

                //Check if pixel is already cached
                if (pixel_to_atlas.find(pixel_size) == pixel_to_atlas.end()) {
                    cachePixel(pixel_size);
                    return pixel_to_atlas[pixel_size];
                }
                else {
                    return pixel_to_atlas[pixel_size];
                }
            }
        }
	}
}
