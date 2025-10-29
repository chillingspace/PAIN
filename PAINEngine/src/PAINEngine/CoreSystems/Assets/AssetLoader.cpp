#include "pch.h"
#include "AssetLoader.h"

#undef max
#undef min

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

        std::unordered_map<GUID, IAsset> Loader::ImportAssetRegistry(std::string const& virtual_path) const {
            std::unordered_map<GUID, IAsset> assets;

            //Use your custom input stream API
            auto fileStream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!fileStream || !fileStream->good()) {
                throw std::runtime_error("Failed to open asset registry file");
            }

            //Read entire file into a buffer
            std::vector<uint8_t> buffer(fileStream->size());
            size_t totalRead = 0;
            while (totalRead < buffer.size()) {
                size_t chunk = fileStream->read(buffer.data() + totalRead, buffer.size() - totalRead);
                if (chunk == 0) break;
                totalRead += chunk;
            }
            buffer.resize(totalRead);

            //Parse JSON from buffer (assumes UTF-8 text)
            nlohmann::json json_package;
            try {
                json_package = nlohmann::json::parse(buffer.begin(), buffer.end());
            }
            catch (const std::exception& e) {
                PN_CORE_ERROR("JSON parse error: {}", e.what());
                return assets;
            }

            //Original asset registry parsing logic
            for (auto it = json_package.begin(); it != json_package.end(); ++it) {
                GUID guid = GUID(it.key());
                const auto& obj = it.value();

                IAsset asset;
                asset.guid = guid;
                asset.type = stringToAssetType(obj["type"]);
                asset.name = obj.value("name", "");
                asset.relative_path = obj.value("relative_path", "");
                assets[guid] = asset;
            }
            fileStream = nullptr;
            return assets;
        }

#ifdef PN_PLATFORM_ANDROID
        void Loader::extractASTC(std::string const& virtual_path, std::shared_ptr<Texture> tex) const {
            // Open with custom file stream
            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good())
                throw std::runtime_error("Failed to open ASTC file: " + virtual_path);

            std::vector<uint8_t> data(stream->size());
            size_t read = stream->read(data.data(), data.size());
            if (read != data.size())
                throw std::runtime_error("Failed to read full ASTC file: " + virtual_path);

            if (data.size() < 16) throw std::runtime_error("ASTC header too small.");
            const uint8_t* header = data.data();

            if (header[0] != 0x13 || header[1] != 0xAB || header[2] != 0xA1 || header[3] != 0x5C)
                throw std::runtime_error("Not an ASTC file!");

            uint8_t blockDimX = header[4], blockDimY = header[5], blockDimZ = header[6];
            if (blockDimZ != 1)
                throw std::runtime_error("Only 2D ASTC textures are supported");

            uint32_t width = header[7] | (header[8] << 8) | (header[9] << 16);
            uint32_t height = header[10] | (header[11] << 8) | (header[12] << 16);
            tex->width = width;
            tex->height = height;
            tex->mips = 1;

            // OpenGL format mapping
            GLenum astcFormat = 0;
            if (blockDimX == 4 && blockDimY == 4) astcFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
            else if (blockDimX == 5 && blockDimY == 4) astcFormat = GL_COMPRESSED_RGBA_ASTC_5x4_KHR;
            else if (blockDimX == 5 && blockDimY == 5) astcFormat = GL_COMPRESSED_RGBA_ASTC_5x5_KHR;
            else if (blockDimX == 6 && blockDimY == 5) astcFormat = GL_COMPRESSED_RGBA_ASTC_6x5_KHR;
            else if (blockDimX == 6 && blockDimY == 6) astcFormat = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
            else if (blockDimX == 8 && blockDimY == 5) astcFormat = GL_COMPRESSED_RGBA_ASTC_8x5_KHR;
            else if (blockDimX == 8 && blockDimY == 6) astcFormat = GL_COMPRESSED_RGBA_ASTC_8x6_KHR;
            else if (blockDimX == 8 && blockDimY == 8) astcFormat = GL_COMPRESSED_RGBA_ASTC_8x8_KHR;
            else if (blockDimX == 10 && blockDimY == 5) astcFormat = GL_COMPRESSED_RGBA_ASTC_10x5_KHR;
            else if (blockDimX == 10 && blockDimY == 6) astcFormat = GL_COMPRESSED_RGBA_ASTC_10x6_KHR;
            else if (blockDimX == 10 && blockDimY == 8) astcFormat = GL_COMPRESSED_RGBA_ASTC_10x8_KHR;
            else if (blockDimX == 10 && blockDimY == 10) astcFormat = GL_COMPRESSED_RGBA_ASTC_10x10_KHR;
            else if (blockDimX == 12 && blockDimY == 10) astcFormat = GL_COMPRESSED_RGBA_ASTC_12x10_KHR;
            else if (blockDimX == 12 && blockDimY == 12) astcFormat = GL_COMPRESSED_RGBA_ASTC_12x12_KHR;
            else throw std::runtime_error("Unsupported ASTC block size: " + std::to_string(blockDimX) + "x" + std::to_string(blockDimY));

            tex->format = TextureFormat::ASTC;
            tex->glTexFormat = astcFormat;
            tex->data.assign(data.begin() + 16, data.end()); // Only texture data

            stream = nullptr;
        }
#else
        void Loader::extractDDS(std::string const& virtual_path, std::shared_ptr<Texture> tex) const {
            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good()) throw std::runtime_error("Failed to open DDS file: " + virtual_path);

            std::vector<uint8_t> data(stream->size());
            size_t read = stream->read(data.data(), data.size());
            if (read != data.size()) throw std::runtime_error("Failed to read full DDS file: " + virtual_path);

            size_t offset = 0;
            if (data.size() < 4) throw std::runtime_error("DDS file data too small for magic number!");
            if (std::memcmp(data.data(), "DDS ", 4) != 0) throw std::runtime_error("Not a DDS file!");
            offset += 4;

            if (data.size() < offset + 124) throw std::runtime_error("DDS file header too small");
            const uint32_t* header = reinterpret_cast<const uint32_t*>(data.data() + offset);

            tex->height = header[2];
            tex->width = header[3];
            uint32_t mipMapCount = header[7] ? header[7] : 1;
            tex->mips = mipMapCount;

            uint32_t pixelFormatFlags = header[19];
            bool isDX10 = (header[20] == 0x30315844);
            offset += 124;

            if (isDX10) {
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
                    tex->glTexFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
                }
                else if (dxgiFormat == 97) {
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                }
                else {
                    throw std::runtime_error("Unsupported DXGI format: " + std::to_string(dxgiFormat));
                }
                offset += 20;
            }
            else {
                throw std::runtime_error("Legacy DDS format detected. BC7 requires DX10 header!");
            }

            tex->data.clear();
            tex->mipOffsets.clear();

            int mipW = tex->width;
            int mipH = tex->height;
            for (uint32_t mip = 0; mip < mipMapCount; ++mip) {
                int blocks_w = (mipW + 3) / 4;
                int blocks_h = (mipH + 3) / 4;
                size_t mipSize = blocks_w * blocks_h * 16;

                tex->mipOffsets.push_back(tex->data.size());
                size_t currentOffset = tex->data.size();
                if (data.size() < offset + mipSize)
                    throw std::runtime_error("DDS file too small for mipmap level " + std::to_string(mip));

                tex->data.resize(tex->data.size() + mipSize);
                std::memcpy(tex->data.data() + currentOffset, data.data() + offset, mipSize);

                offset += mipSize;
                mipW = std::max(1, mipW / 2);
                mipH = std::max(1, mipH / 2);
            }

            stream = nullptr;
        }
#endif

        std::shared_ptr<Texture> Loader::ImportTexture(std::string const& virtual_path) const {

            //Create default texture
            auto tex = std::make_shared<Texture>();

#ifdef PN_PLATFORM_ANDROID
            extractASTC(virtual_path, tex);

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

            // Use blockDimX/Y from extracted tex data
            uint8_t blockDimX = 4, blockDimY = 4;
            if (!tex->data.empty() && tex->width && tex->height) {
                // Redundant, since extractASTC already checks, but could parse again if needed from header[4,5]
                blockDimX = 4; // Set to correct value if you pass it through
                blockDimY = 4;
            }
            GLsizei blockW = (tex->width + blockDimX - 1) / blockDimX;
            GLsizei blockH = (tex->height + blockDimY - 1) / blockDimY;
            GLsizei imageSize = blockW * blockH * 16;

            glCompressedTexImage2D(GL_TEXTURE_2D, 0, tex->glTexFormat, tex->width, tex->height, 0, imageSize, tex->data.data());

            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                throw std::runtime_error("OpenGL error uploading ASTC texture.");
            }

            glBindTexture(GL_TEXTURE_2D, 0);
#else
            // Extract DDS (Windows, etc.)
            extractDDS(virtual_path, tex);

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

		std::shared_ptr<Model> Loader::ImportModel(std::string const& virtual_path) const {
            // Open custom stream, supports both APK assets and regular files
            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good())
                throw std::runtime_error("Failed to open model file: " + virtual_path);

            // Preallocate file size, read full file into buffer in one go for parsing
            std::vector<uint8_t> data(stream->size());
            size_t read = stream->read(data.data(), data.size());
            if (read != data.size())
                throw std::runtime_error("Failed to read full model file: " + virtual_path);

            Model asset;
            size_t offset = 0;

            auto require = [&](size_t n) {
                if (offset + n > data.size())
                    throw std::runtime_error("Unexpected end of model file data!");
                };
            auto readMem = [&](void* dst, size_t n) {
                require(n);
                std::memcpy(dst, data.data() + offset, n);
                offset += n;
                };

            // Vertices/Indices
            uint32_t vtxCount = 0, idxCount = 0;
            readMem(&vtxCount, sizeof(vtxCount));
            readMem(&idxCount, sizeof(idxCount));
            asset.vertices.resize(vtxCount);
            asset.indices.resize(idxCount);
            readMem(asset.vertices.data(), vtxCount * sizeof(Vertex));
            readMem(asset.indices.data(), idxCount * sizeof(uint32_t));

            // Skeleton (bones)
            uint32_t boneCount = 0;
            readMem(&boneCount, sizeof(boneCount));
            asset.skeleton.resize(boneCount);
            for (Bone& b : asset.skeleton) {
                uint32_t nameLen = 0;
                readMem(&nameLen, sizeof(nameLen));
                b.name.resize(nameLen);
                readMem(&b.name[0], nameLen);
                readMem(&b.parent, sizeof(b.parent));
                readMem(&b.bindPose, sizeof(glm::mat4));
            }

            // Skinning Weights
            asset.weights.resize(vtxCount);
            for (auto& vweights : asset.weights) {
                uint32_t count = 0;
                readMem(&count, sizeof(count));
                vweights.resize(count);
                readMem(vweights.data(), count * sizeof(BoneWeight));
            }

            // Animations
            uint32_t animCount = 0;
            readMem(&animCount, sizeof(animCount));
            asset.animations.resize(animCount);
            for (AnimationClip& anim : asset.animations) {
                uint32_t nameLen = 0;
                readMem(&nameLen, sizeof(nameLen));
                anim.name.resize(nameLen);
                readMem(&anim.name[0], nameLen);
                readMem(&anim.duration, sizeof(anim.duration));
                uint32_t trackCount = 0;
                readMem(&trackCount, sizeof(trackCount));
                anim.tracks.resize(trackCount);
                for (AnimationTrack& track : anim.tracks) {
                    uint32_t boneLen = 0;
                    readMem(&boneLen, sizeof(boneLen));
                    track.boneName.resize(boneLen);
                    readMem(&track.boneName[0], boneLen);
                    uint32_t keyCount = 0;
                    readMem(&keyCount, sizeof(keyCount));
                    track.keys.resize(keyCount);
                    readMem(track.keys.data(), keyCount * sizeof(AnimationKey));
                }
            }

            // Materials
            uint32_t matCount = 0;
            readMem(&matCount, sizeof(matCount));
            asset.materials.resize(matCount);
            for (Material& mat : asset.materials) {
                uint32_t nameLen = 0;
                readMem(&nameLen, sizeof(nameLen));
                mat.name.resize(nameLen);
                readMem(&mat.name[0], nameLen);
                uint32_t diffLen = 0;
                readMem(&diffLen, sizeof(diffLen));
                mat.diffuseMap.resize(diffLen);
                readMem(&mat.diffuseMap[0], diffLen);
                // Read normal/specular etc. here if you add them to export.
            }

            return std::make_shared<Model>(std::move(asset));
		}
	}
}