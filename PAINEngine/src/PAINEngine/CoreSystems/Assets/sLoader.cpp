#ifdef PN_PLATFORM_WINDOWS

#include "pch.h"
#include "sLoader.h"

namespace PAIN {
    namespace Loader {
        Texture RenderLoader::loadTexture(const std::string& path_to_texture_desc)
        {
            // Determine platform directories
#if defined(PN_PLATFORM_ANDROID)
            std::filesystem::path compiled_dir = "assets/Textures/Compiled_Textures_Android";
            std::string compiled_ext = ".astc";
#elif defined(PN_PLATFORM_WINDOWS)
            std::filesystem::path compiled_dir = "assets/Textures/Compiled_Textures_Windows";
            std::string compiled_ext = ".dds";
#else
    // Fallback
            std::filesystem::path compiled_dir = "assets/Textures";
            std::string compiled_ext = "";
#endif

            
            std::filesystem::path desc_path(path_to_texture_desc);
            // Derive texture base name from descriptor file
            auto data = Compiler::readDescFile(path_to_texture_desc);
            if (!data.contains("asset_id")) {
                throw std::runtime_error("Texture descriptor missing 'asset_id': " + path_to_texture_desc);
            }

            std::string tex_name = data["asset_id"].get<std::string>();

            // Construct full path for compiled texture
            std::filesystem::path compiled_tex_path = compiled_dir / (tex_name + compiled_ext);

            GLuint textureID = 0;
            bool loaded = false;

            Texture tex{};
            if (std::filesystem::exists(compiled_tex_path)) {
                // Load compressed texture file (.astc or .dds)
                loaded = loadCompressedTextureFromFile(compiled_tex_path.string(), tex);
            }

            if (!loaded)
            {
               PN_CORE_ERROR("Failed to load compiled texture: {}", compiled_tex_path.string());
            }

            return tex;
        }

        bool RenderLoader::loadFileToMemory(const std::string& filepath, std::vector<unsigned char>& out_buffer)
        {
            std::ifstream file(filepath, std::ios::binary | std::ios::ate);
            if (!file) return false;
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            out_buffer.resize(size);
            return file.read((char*)out_buffer.data(), size).good();
        }

        bool RenderLoader::loadASTCTexture(const unsigned char* data, size_t data_size, GLuint& out_texID, int& out_width, int& out_height)
        {
            // Example using fixed format and dummy values:
            out_width = 256;   // read from header
            out_height = 256;  // read from header

            glGenTextures(1, &out_texID);
            glBindTexture(GL_TEXTURE_2D, out_texID);

            // simplified example � supply real image size and data
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_ASTC_4x4_KHR, out_width, out_height, 0, (GLsizei)(data_size - 16), data + 16);

            // Setup filters
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            return true;
        }

        bool RenderLoader::loadDDSTexture(const std::string& filepath, GLuint& out_texID, int& out_width, int& out_height, int& out_depth)
        {
            //gli::texture Texture = gli::load(filepath);
            //if (Texture.empty()) {
            //    PN_CORE_WARN("Failed to load DDS texture: {}", filepath);
            //    return false;
            //}

            //gli::gl GL(gli::gl::PROFILE_GL33);
            //gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
            //GLenum target = GL.translate(Texture.target());

            //glGenTextures(1, &out_texID);
            //glBindTexture(target, out_texID);

            //glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, 0);
            //glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
            //glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
            //    Texture.levels() > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            //glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            //glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
            //glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);

            //if (target == GL_TEXTURE_3D) {
            //    glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);
            //}

            //glm::tvec3<GLsizei> const Extent(Texture.extent());
            //out_width = static_cast<int>(Extent.x);
            //out_height = static_cast<int>(Extent.y);
            //out_depth = static_cast<int>(Extent.z);

            //// Upload texture data for each mipmap level
            //for (std::size_t Level = 0; Level < Texture.levels(); ++Level)
            //{
            //    glm::tvec3<GLsizei> LevelExtent(Texture.extent(Level));

            //    if (target == GL_TEXTURE_2D)
            //    {
            //        if (gli::is_compressed(Texture.format()))
            //        {
            //            glCompressedTexImage2D(
            //                target,
            //                static_cast<GLint>(Level),
            //                Format.Internal,
            //                LevelExtent.x,
            //                LevelExtent.y,
            //                0,
            //                static_cast<GLsizei>(Texture.size(Level)),
            //                Texture.data(0, 0, Level)
            //            );
            //        }
            //        else
            //        {
            //            glTexImage2D(
            //                target,
            //                static_cast<GLint>(Level),
            //                Format.Internal,
            //                LevelExtent.x,
            //                LevelExtent.y,
            //                0,
            //                Format.External,
            //                Format.Type,
            //                Texture.data(0, 0, Level)
            //            );
            //        }
            //    }
            //    else if (target == GL_TEXTURE_3D)
            //    {
            //        if (gli::is_compressed(Texture.format()))
            //        {
            //            glCompressedTexImage3D(
            //                target,
            //                static_cast<GLint>(Level),
            //                Format.Internal,
            //                LevelExtent.x,
            //                LevelExtent.y,
            //                LevelExtent.z,
            //                0,
            //                static_cast<GLsizei>(Texture.size(Level)),
            //                Texture.data(0, 0, Level)
            //            );
            //        }
            //        else
            //        {
            //            glTexImage3D(
            //                target,
            //                static_cast<GLint>(Level),
            //                Format.Internal,
            //                LevelExtent.x,
            //                LevelExtent.y,
            //                LevelExtent.z,
            //                0,
            //                Format.External,
            //                Format.Type,
            //                Texture.data(0, 0, Level)
            //            );
            //        }
            //    }
            //    else if (target == GL_TEXTURE_CUBE_MAP)
            //    {
            //        PN_CORE_WARN("Cube map textures not yet supported: {}", filepath);
            //        glDeleteTextures(1, &out_texID);
            //        return false;
            //    }
            //}

            //GLenum error = glGetError();
            //if (error != GL_NO_ERROR) {
            //    PN_CORE_ERROR("OpenGL error while loading DDS texture: {} (error: {})", filepath, error);
            //    glDeleteTextures(1, &out_texID);
            //    return false;
            //}

            return true;
        }



        bool RenderLoader::loadCompressedTextureFromFile(const std::string& filepath, Texture& out_texture)
        {
            std::vector<unsigned char> fileData;
            if (!loadFileToMemory(filepath, fileData)) {
                PN_CORE_WARN("Failed to open compressed texture file: {}", filepath);
                return false;
            }

            GLuint textureID = 0;
            int width = 0, height = 0, depth = 1;
            GLenum target = GL_TEXTURE_2D;
            bool result = false;

            if (filepath.ends_with(".astc")) {
                result = loadASTCTexture(fileData.data(), fileData.size(), textureID, width, height);
            }
            else if (filepath.ends_with(".dds")) {
                result = loadDDSTexture(filepath, textureID, width, height, depth);
                target = (depth > 1) ? GL_TEXTURE_3D : GL_TEXTURE_2D;
            }
            else {
               PN_CORE_WARN("Unsupported compressed texture extension: {}", filepath);
                return false;
            }

            if (!result) {
                PN_CORE_WARN("Compressed texture loading failed: {}", filepath);
                return false;
            }

            out_texture = Texture(textureID, ivec3(width, height, depth), target, filepath);
            return true;
        }

    } // namespace Loader
} // namespace PAIN

#endif


