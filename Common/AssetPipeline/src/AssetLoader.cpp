
#include "AssetLoader.h"

namespace PAIN {
	namespace Assets {

        void Loader::RegisterLoader(Type const& type, LoaderFunc const& func) {

            //Check if type already regist
            if (asset_loader.find(type) != asset_loader.end()) {
                std::cout << "Loader of type: " << assetTypeToString(type) << " already registered. Overriding with new loader." << std::endl;
            }

            asset_loader[type] = func;
        }

        LoaderFunc Loader::GetLoader(Type const& type) const {
            
            //Check if loader exists
            if (!CheckLoader(type)) {
                throw std::runtime_error("Loader does not exist! Unable to get loader function!");
            }

            return asset_loader.at(type);
        }

        bool Loader::CheckLoader(Type const& type) const {
            if (asset_loader.find(type) != asset_loader.end()) {
                return true;
            }

            return false;
        }

        std::unordered_map<GUID, IAsset> Loader::ImportAssetRegistry(nlohmann::json const& json_package) const {
            std::unordered_map<GUID, IAsset> assets;

            for (auto it = json_package.begin(); it != json_package.end(); ++it) {
                // Parse GUID from key
                GUID guid = GUID(it.key());
                const auto& obj = it.value();

                IAsset asset;
                asset.guid = guid;

                //Parse asset type
                asset.type = stringToAssetType(obj["type"]);
                asset.name = obj.value("name", "");

                //Parse paths
                asset.relative_path = obj.value("relative_path", "");

                //Store in map
                assets[guid] = asset;
            }
            return assets;
        }

#ifdef PN_PLATFORM_ANDROID
        void Loader::extractASTC(std::vector<uint8_t> const& data, std::shared_ptr<Texture> tex) const {
            if (data.size() < 16)
                throw std::runtime_error("ASTC header too small.");
            const uint8_t* header = data.data();

            // Magic number
            if (header[0] != 0x13 || header[1] != 0xAB || header[2] != 0xA1 || header[3] != 0x5C)
                throw std::runtime_error("Not an ASTC file!");

            // Block size
            uint8_t blockDimX = header[4];
            uint8_t blockDimY = header[5];
            uint8_t blockDimZ = header[6];

            // Only 2D textures (blockDimZ == 1)
            if (blockDimZ != 1)
                throw std::runtime_error("Only 2D ASTC textures are supported");

            // Texture size (24-bit little-endian)
            uint32_t width = header[7] | (header[8] << 8) | (header[9] << 16);
            uint32_t height = header[10] | (header[11] << 8) | (header[12] << 16);
            tex->width = width;
            tex->height = height;
            tex->mips = 1; // Change if you want to support ASTC mipmaps

            // Set correct OpenGL enum from blockDims (KHR extension)
            GLenum astcFormat = 0;
            if (blockDimX == 4 && blockDimY == 4)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
            else if (blockDimX == 5 && blockDimY == 4)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
            else if (blockDimX == 5 && blockDimY == 5)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
            else if (blockDimX == 6 && blockDimY == 5)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
            else if (blockDimX == 6 && blockDimY == 6)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
            else if (blockDimX == 8 && blockDimY == 5)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
            else if (blockDimX == 8 && blockDimY == 6)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
            else if (blockDimX == 8 && blockDimY == 8)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
            else if (blockDimX == 10 && blockDimY == 5)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
            else if (blockDimX == 10 && blockDimY == 6)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
            else if (blockDimX == 10 && blockDimY == 8)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
            else if (blockDimX == 10 && blockDimY == 10)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
            else if (blockDimX == 12 && blockDimY == 10)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
            else if (blockDimX == 12 && blockDimY == 12)
                astcFormat = GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
            else
                throw std::runtime_error("Unsupported ASTC block size: " + std::to_string(blockDimX) + "x" + std::to_string(blockDimY));

            tex->format = TextureFormat::ASTC; // use your enum
            tex->glTexFormat = astcFormat;

            // Copy compressed data (rest of the file)
            tex->data.assign(data.begin() + 16, data.end());
        }
#else
        void Loader::extractDDS(std::vector<uint8_t> const& data, std::shared_ptr<Texture> tex) const {
            size_t offset = 0;

            // Read magic number
            if (data.size() < 4)
                throw std::runtime_error("DDS file data too small for magic number!");
            if (std::memcmp(data.data(), "DDS ", 4) != 0)
                throw std::runtime_error("Not a DDS file!");
            offset += 4;

            // Read DDS header (124 bytes)
            if (data.size() < offset + 124)
                throw std::runtime_error("DDS file header too small");
            const uint32_t* header = reinterpret_cast<const uint32_t*>(data.data() + offset);

            tex->height = header[2];
            tex->width = header[3];
            uint32_t mipMapCount = header[7] ? header[7] : 1;
            tex->mips = mipMapCount;

            // Check for DX10 extended header
            uint32_t pixelFormatFlags = header[19];
            bool isDX10 = (header[20] == 0x30315844); // "DX10" in little-endian

            offset += 124;

            if (isDX10) {
                // Read complete DX10 header (20 bytes = 5 uint32_t values)
                if (data.size() < offset + 20)
                    throw std::runtime_error("DDS file too small for DX10 header");
                const uint32_t* dx10Header = reinterpret_cast<const uint32_t*>(data.data() + offset);

                uint32_t dxgiFormat = dx10Header[0];
                if (dxgiFormat == 98) {
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                }
                else if (dxgiFormat == 99) {
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB; // sRGB variant
                }
                else if (dxgiFormat == 97) {
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                }
                else {
                    throw std::runtime_error("Unsupported DXGI format: " + std::to_string(dxgiFormat) + " (expected BC7: 97/98/99)");
                }
                offset += 20;
            }
            else {
                throw std::runtime_error("Legacy DDS format detected. BC7 requires DX10 header!");
            }

            // Prepare data storage
            tex->data.clear();
            tex->mipOffsets.clear();

            // Read all mipmap levels WITHOUT flipping
            int mipW = tex->width;
            int mipH = tex->height;
            for (uint32_t mip = 0; mip < mipMapCount; ++mip) {
                int blocks_w = (mipW + 3) / 4;
                int blocks_h = (mipH + 3) / 4;
                size_t mipSize = blocks_w * blocks_h * 16; // BC7

                tex->mipOffsets.push_back(tex->data.size());

                size_t currentOffset = tex->data.size();
                // Check if enough bytes left in data
                if (data.size() < offset + mipSize)
                    throw std::runtime_error("DDS file too small for mipmap level " + std::to_string(mip));

                tex->data.resize(tex->data.size() + mipSize);
                std::memcpy(tex->data.data() + currentOffset, data.data() + offset, mipSize);

                offset += mipSize;

                mipW = std::max(1, mipW / 2);
                mipH = std::max(1, mipH / 2);
            }
        }
#endif

        std::shared_ptr<Texture> Loader::ImportTexture(std::vector<uint8_t> const& data) const {

            //Create default texture
            auto tex = std::make_shared<Texture>();

#ifdef PN_PLATFORM_ANDROID
            extractASTC(data, tex);

            glGenTextures(1, &tex->gl_texture);
            glBindTexture(GL_TEXTURE_2D, tex->gl_texture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex->mips - 1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            // If you only upload a single mip (most ASTC assets), use non-mipmap filtering:
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Anisotropic filtering for highest quality (guard extension)
            GLfloat maxAniso = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

            uint8_t blockDimX = data[4], blockDimY = data[5];
            GLsizei blockW = (tex->width + blockDimX - 1) / blockDimX;
            GLsizei blockH = (tex->height + blockDimY - 1) / blockDimY;
            GLsizei imageSize = blockW * blockH * 16; // 16 bytes per block

            glCompressedTexImage2D(GL_TEXTURE_2D, 0, tex->glTexFormat, tex->width, tex->height, 0, imageSize, tex->data.data());

            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                throw std::runtime_error("OpenGL error uploading ASTC texture.");
            }

            glBindTexture(GL_TEXTURE_2D, 0);
#else
            // Extract DDS (Windows, etc.)
            extractDDS(data, tex);

            glGenTextures(1, &tex->gl_texture);
            glBindTexture(GL_TEXTURE_2D, tex->gl_texture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex->mips - 1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            GLfloat maxAniso = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

            int mipW = tex->width;
            int mipH = tex->height;

            for (uint32_t mip = 0; mip < tex->mips; ++mip) {
                int blocks_w = (mipW + 3) / 4;
                int blocks_h = (mipH + 3) / 4;
                size_t mipSize = blocks_w * blocks_h * 16;
                size_t offset = tex->mipOffsets[mip];

                glCompressedTexImage2D(
                    GL_TEXTURE_2D,
                    mip,
                    tex->glTexFormat,
                    mipW,
                    mipH,
                    0,
                    mipSize,
                    tex->data.data() + offset
                );

                GLenum err = glGetError();
                if (err != GL_NO_ERROR) {
                    throw std::runtime_error("OpenGL error uploading mip " +
                        std::to_string(mip) + ": 0x" +
                        std::to_string(err));
                }

                mipW = std::max(1, mipW / 2);
                mipH = std::max(1, mipH / 2);
            }
#endif
            glBindTexture(GL_TEXTURE_2D, 0);
            return tex;
        }

		std::shared_ptr<Model> Loader::ImportModel(std::vector<uint8_t> const& data) const {
            Model asset;
            size_t offset = 0;

            auto require = [&](size_t n) {
                if (offset + n > data.size())
                    throw std::runtime_error("Unexpected end of model file data!");
                };
            auto read = [&](void* dst, size_t n) {
                require(n);
                std::memcpy(dst, data.data() + offset, n);
                offset += n;
                };

            //Vertices/indices
            uint32_t vtxCount = 0, idxCount = 0;
            read((char*)&vtxCount, sizeof(vtxCount));
            read((char*)&idxCount, sizeof(idxCount));
            asset.vertices.resize(vtxCount);
            asset.indices.resize(idxCount);
            read((char*)asset.vertices.data(), vtxCount * sizeof(Vertex));
            read((char*)asset.indices.data(), idxCount * sizeof(uint32_t));

            //Skeleton (bones)
            uint32_t boneCount = 0;
            read((char*)&boneCount, sizeof(boneCount));
            asset.skeleton.resize(boneCount);
            for (Bone& b : asset.skeleton) {
                uint32_t nameLen = 0;
                read((char*)&nameLen, sizeof(nameLen));
                b.name.resize(nameLen);
                read(&b.name[0], nameLen);
                read((char*)&b.parent, sizeof(b.parent));
                read((char*)&b.bindPose, sizeof(glm::mat4));
            }

            //Skinning Weights
            asset.weights.resize(vtxCount);
            for (auto& vweights : asset.weights) {
                uint32_t count = 0;
                read((char*)&count, sizeof(count));
                vweights.resize(count);
                read((char*)vweights.data(), count * sizeof(BoneWeight));
            }

            //Animations
            uint32_t animCount = 0;
            read((char*)&animCount, sizeof(animCount));
            asset.animations.resize(animCount);
            for (AnimationClip& anim : asset.animations) {
                uint32_t nameLen = 0;
                read((char*)&nameLen, sizeof(nameLen));
                anim.name.resize(nameLen);
                read(&anim.name[0], nameLen);
                read((char*)&anim.duration, sizeof(anim.duration));
                uint32_t trackCount = 0;
                read((char*)&trackCount, sizeof(trackCount));
                anim.tracks.resize(trackCount);
                for (AnimationTrack& track : anim.tracks) {
                    uint32_t boneLen = 0;
                    read((char*)&boneLen, sizeof(boneLen));
                    track.boneName.resize(boneLen);
                    read(&track.boneName[0], boneLen);
                    uint32_t keyCount = 0;
                    read((char*)&keyCount, sizeof(keyCount));
                    track.keys.resize(keyCount);
                    read((char*)track.keys.data(), keyCount * sizeof(AnimationKey));
                }
            }

            //Materials
            uint32_t matCount = 0;
            read((char*)&matCount, sizeof(matCount));
            asset.materials.resize(matCount);
            for (Material& mat : asset.materials) {
                uint32_t nameLen = 0;
                read((char*)&nameLen, sizeof(nameLen));
                mat.name.resize(nameLen);
                read(&mat.name[0], nameLen);
                uint32_t diffLen = 0;
                read((char*)&diffLen, sizeof(diffLen));
                mat.diffuseMap.resize(diffLen);
                read(&mat.diffuseMap[0], diffLen);
                // Read normal/specular etc. here if you add them to export.
            }

            return std::make_shared<Model>(asset);
		}
	}
}