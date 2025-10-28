
#include "AssetLoader.h"

namespace PAIN {
	namespace Assets {

        std::unordered_map<GUID, IAsset> Loader::ImportAssetRegistry(std::filesystem::path const& path) {
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

        void Loader::extractDDS(std::filesystem::path const& path, std::shared_ptr<Texture> tex) {
#ifdef PN_PLATFORM_WINDOWS
            std::ifstream file(path, std::ios::binary);
            char magic[4];
            file.read(magic, 4);
            if (std::memcmp(magic, "DDS ", 4) != 0) throw std::runtime_error("Not a DDS file!");
            file.ignore(8); // size, flags
            uint32_t height, width; file.read((char*)&height, 4); file.read((char*)&width, 4);
            file.ignore(108); // skip rest of header (could extract mips, format here)
            tex->width = width;
            tex->height = height;
            tex->mips = 1; // For simplicity here; parse real mip count in production!
            tex->format = TextureFormat::BC7;
            // Copy whole file (after header) to tex->data
            tex->data.assign(std::istreambuf_iterator<char>(file), {});
            tex->glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            file.close();
#endif
        }

        void Loader::extractASTC(std::filesystem::path const& path, std::shared_ptr<Texture> tex) {
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

        std::shared_ptr<Texture> Loader::ImportTexture(std::filesystem::path const& path) {

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
            if (path.extension() == ".astc") {
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
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            if (tex->format == TextureFormat::BC7) {
#ifdef PN_PLATFORM_WINDOWS
                //BC7 block size (16 bytes/block, 4x4 texel block)
                int blockWidth = (tex->width + 3) / 4, blockHeight = (tex->height + 3) / 4;
                GLsizei imageSize = blockWidth * blockHeight * 16;
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, tex->glTexFormat, tex->width, tex->height, 0, imageSize, tex->data.data());
#endif
            }
            else if (tex->format == TextureFormat::ASTC_4x4) {
#ifdef PN_PLATFORM_ANDROID
                //ASTC 4x4 block size = 16 bytes/block
                int blockWidth = (tex->width + 3) / 4, blockHeight = (tex->height + 3) / 4;
                GLsizei imageSize = blockWidth * blockHeight * 16;
                glCompressedTexImage2D(GL_TEXTURE_2D, 0, tex->glTexFormat, tex->width, tex->height, 0, imageSize, tex->data.data());
#endif
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            return tex;
        }

		std::shared_ptr<Model> Loader::ImportModel(std::filesystem::path const& path) {
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