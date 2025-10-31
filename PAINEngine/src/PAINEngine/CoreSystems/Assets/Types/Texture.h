#pragma once

#ifndef ASSETS_TEXTURE_HPP
#define ASSETS_TEXTURE_HPP

#include "AssetTypes.h"

#ifdef PN_PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

#include "CoreSystems/Path/Path.h"

namespace PAIN {
    namespace Assets {

        //Texture format
        enum class TextureFormat {
            UNKNOWN, BC7, ASTC, // add more as needed
        };

        //Texture class
        struct Texture : public IAsset {
        public:
            int width = 0, height = 0, mips = 1;
            std::vector<size_t> mipOffsets;
#ifdef PN_PLATFORM_ANDROID
            TextureFormat format = TextureFormat::ASTC;
            unsigned int glTexFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
#else
            TextureFormat format = TextureFormat::BC7;
            unsigned int glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
#endif

            std::vector<uint8_t> data;
            GLuint gl_texture = 0;

            ~Texture() { if (gl_texture) glDeleteTextures(1, &gl_texture); }
        };
    }
}

#endif
