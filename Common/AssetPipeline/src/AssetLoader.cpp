
#include "AssetLoader.h"

namespace PAIN {
	namespace Assets {

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

        std::unordered_map<GUID, IAsset> Loader::ImportAssetRegistry(std::filesystem::path const& path) const {
            std::unordered_map<GUID, IAsset> assets;

            //Check if file exists
            if (!std::filesystem::exists(path)) {
                throw std::runtime_error("Asset registry does not exist or invalid path!");
            }

            std::ifstream file(path);
            if (!file.is_open()) {
                throw std::runtime_error("Unable to open path!");
            }

            nlohmann::json registry_json;
            file >> registry_json;

            for (auto it = registry_json.begin(); it != registry_json.end(); ++it) {
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

        void Loader::extractDDS(std::filesystem::path const& path, std::shared_ptr<Texture> tex) const {
#ifdef PN_PLATFORM_WINDOWS
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open DDS file: " + path.string());
            }

            // Read magic number
            char magic[4];
            file.read(magic, 4);
            if (std::memcmp(magic, "DDS ", 4) != 0) {
                throw std::runtime_error("Not a DDS file!");
            }

            // Read DDS header (124 bytes)
            uint32_t header[31];
            file.read((char*)header, sizeof(header));

            tex->height = header[2];
            tex->width = header[3];
            uint32_t mipMapCount = header[7] ? header[7] : 1;
            tex->mips = mipMapCount;

            // Check for DX10 extended header
            uint32_t pixelFormatFlags = header[19];
            bool isDX10 = (header[20] == 0x30315844); // "DX10" in little-endian

            if (isDX10) {
                // Read complete DX10 header (20 bytes = 5 uint32_t values)
                uint32_t dx10Header[5];
                file.read((char*)dx10Header, 20);

                uint32_t dxgiFormat = dx10Header[0];

                // DXGI_FORMAT_BC7_TYPELESS = 97
                // DXGI_FORMAT_BC7_UNORM = 98
                // DXGI_FORMAT_BC7_UNORM_SRGB = 99
                if (dxgiFormat == 98) {
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                }
                else if (dxgiFormat == 99) {
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB; // sRGB variant
                }
                else if (dxgiFormat == 97) {
                    // Typeless - default to linear
                    tex->format = TextureFormat::BC7;
                    tex->glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                }
                else {
                    throw std::runtime_error("Unsupported DXGI format: " + std::to_string(dxgiFormat) + " (expected BC7: 97/98/99)");
                }
                // File cursor is already at the correct position after reading DX10 header
            }
            else {
                // Legacy DDS format - check FourCC for BC7
                throw std::runtime_error("Legacy DDS format detected. BC7 requires DX10 header!");
            }

            // Prepare data storage
            tex->data.clear();
            tex->mipOffsets.clear();

            // Read all mipmap levels WITHOUT flipping
            int mipW = tex->width;
            int mipH = tex->height;

            for (uint32_t mip = 0; mip < mipMapCount; ++mip) {
                // Calculate block dimensions and size
                int blocks_w = (mipW + 3) / 4;
                int blocks_h = (mipH + 3) / 4;
                size_t mipSize = blocks_w * blocks_h * 16; // BC7 always uses 16 bytes per block

                // Store offset for this mip level
                tex->mipOffsets.push_back(tex->data.size());

                // Resize and read directly into final buffer
                size_t currentOffset = tex->data.size();
                tex->data.resize(tex->data.size() + mipSize);

                file.read(reinterpret_cast<char*>(tex->data.data() + currentOffset), mipSize);

                if (!file) {
                    throw std::runtime_error("Failed to read mipmap level " + std::to_string(mip) +
                        " (expected " + std::to_string(mipSize) + " bytes)");
                }

                // Calculate next mip dimensions
                mipW = std::max(1, mipW / 2);
                mipH = std::max(1, mipH / 2);
            }

            file.close();
#endif
        }

        void Loader::extractASTC(std::filesystem::path const& path, std::shared_ptr<Texture> tex) const {
#ifdef PN_PLATFORM_ANDROID
            // --- Minimal ASTC header parse (supports .astc 4x4 only here) ---
            std::ifstream file(path, std::ios::binary);
            uint8_t header[16];
            file.read((char*)header, 16);
            if (header[0] != 0x13 || header[1] != 0xAB || header[2] != 0xA1 || header[3] != 0x5C)
                throw std::runtime_error("Not an ASTC file!");
            tex->width = header[7] | (header[8] << 8) | (header[9] << 16);
            tex->height = header[10] | (header[11] << 8) | (header[12] << 16);
            tex->mips = 1; // Most shipped ASTC is 1 mip, but you can parse more if included.
            tex->format = TextureFormat::ASTC_4x4;
            // Copy the rest of the file (compressed blocks)
            tex->data.assign(std::istreambuf_iterator<char>(file), {});
            tex->glTexFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
            file.close();
#endif
        }

        std::shared_ptr<Texture> Loader::ImportTexture(std::filesystem::path const& path) const {

            //Check if path exists
            if (!std::filesystem::exists(path)) {
                throw std::runtime_error("Texture path does not exist!");
            }

            //Create default texture
            auto tex = std::make_shared<Texture>();

            //Filter extensions
            if (path.extension() == ".dds") {
#ifdef PN_PLATFORM_WINDOWS
                //Extract DDS Texture
                extractDDS(path, tex);
#endif
            }
            else if (path.extension() == ".astc") {
#ifdef PN_PLATFORM_ANDROID
                //Extract DDS Texture
                extractASTC(path, tex);
#endif
            }
            else {
                throw std::runtime_error("Unknown texture format!");
            }

            //Immediate upload
            glGenTextures(1, &tex->gl_texture);
            glBindTexture(GL_TEXTURE_2D, tex->gl_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex->mips - 1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Anisotropic filtering (essential for quality)
            GLfloat maxAniso = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

            if (tex->format == TextureFormat::BC7) {
#ifdef PN_PLATFORM_WINDOWS
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
            }
            else if (tex->format == TextureFormat::ASTC_4x4) {
#ifdef PN_PLATFORM_ANDROID
                //ASTC 4x4 block size = 16 bytes/block
                int blockWidth = (tex->width + 3) / 4, blockHeight = (tex->height + 3) / 4;
                GLsizei imageSize = blockWidth * blockHeight * 16;
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, tex->glTexFormat, tex->width, tex->height, 0, imageSize, tex->data.data());

                GLenum err = glGetError();
                if (err != GL_NO_ERROR) {
                    throw std::runtime_error("OpenGL error occurred uploading texture.");
                }
#endif
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            return tex;
        }

		std::shared_ptr<Model> Loader::ImportModel(std::filesystem::path const& path) const {
            Model asset;

            //Double check if file is valid
            if (path.extension() != ".mesh" || !std::filesystem::exists(path)) {
                throw std::runtime_error("Invalid file type for importing model!");
            }

            //Read model file
            std::ifstream in(path, std::ios::binary);
            if (!in) {
                throw std::runtime_error("Failed to read file for importing model!");
            }

            //Vertices/indices
            uint32_t vtxCount = 0, idxCount = 0;
            in.read((char*)&vtxCount, sizeof(vtxCount));
            in.read((char*)&idxCount, sizeof(idxCount));
            asset.vertices.resize(vtxCount);
            asset.indices.resize(idxCount);
            in.read((char*)asset.vertices.data(), vtxCount * sizeof(Vertex));
            in.read((char*)asset.indices.data(), idxCount * sizeof(uint32_t));

            //Skeleton (bones)
            uint32_t boneCount = 0;
            in.read((char*)&boneCount, sizeof(boneCount));
            asset.skeleton.resize(boneCount);
            for (Bone& b : asset.skeleton) {
                uint32_t nameLen = 0;
                in.read((char*)&nameLen, sizeof(nameLen));
                b.name.resize(nameLen);
                in.read(&b.name[0], nameLen);
                in.read((char*)&b.parent, sizeof(b.parent));
                in.read((char*)&b.bindPose, sizeof(glm::mat4));
            }

            //Skinning Weights
            asset.weights.resize(vtxCount);
            for (auto& vweights : asset.weights) {
                uint32_t count = 0;
                in.read((char*)&count, sizeof(count));
                vweights.resize(count);
                in.read((char*)vweights.data(), count * sizeof(BoneWeight));
            }

            //Animations
            uint32_t animCount = 0;
            in.read((char*)&animCount, sizeof(animCount));
            asset.animations.resize(animCount);
            for (AnimationClip& anim : asset.animations) {
                uint32_t nameLen = 0;
                in.read((char*)&nameLen, sizeof(nameLen));
                anim.name.resize(nameLen);
                in.read(&anim.name[0], nameLen);
                in.read((char*)&anim.duration, sizeof(anim.duration));
                uint32_t trackCount = 0;
                in.read((char*)&trackCount, sizeof(trackCount));
                anim.tracks.resize(trackCount);
                for (AnimationTrack& track : anim.tracks) {
                    uint32_t boneLen = 0;
                    in.read((char*)&boneLen, sizeof(boneLen));
                    track.boneName.resize(boneLen);
                    in.read(&track.boneName[0], boneLen);
                    uint32_t keyCount = 0;
                    in.read((char*)&keyCount, sizeof(keyCount));
                    track.keys.resize(keyCount);
                    in.read((char*)track.keys.data(), keyCount * sizeof(AnimationKey));
                }
            }

            //Materials
            uint32_t matCount = 0;
            in.read((char*)&matCount, sizeof(matCount));
            asset.materials.resize(matCount);
            for (Material& mat : asset.materials) {
                uint32_t nameLen = 0;
                in.read((char*)&nameLen, sizeof(nameLen));
                mat.name.resize(nameLen);
                in.read(&mat.name[0], nameLen);
                uint32_t diffLen = 0;
                in.read((char*)&diffLen, sizeof(diffLen));
                mat.diffuseMap.resize(diffLen);
                in.read(&mat.diffuseMap[0], diffLen);
                // Read normal/specular etc. here if you add them to export.
            }

            in.close();
            return std::make_shared<Model>(asset);
		}
	}
}