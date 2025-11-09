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
            
            //Check if loader mesh_id
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

        std::unordered_map<GUID, std::shared_ptr<IAsset>> Loader::ImportAssetRegistry(std::string const& virtual_path) const {
            std::unordered_map<GUID, std::shared_ptr<IAsset>> assets;

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

                //ensure GUID is valid
                if (!guid.IsValid()) continue;

                const auto& obj = it.value();

                IAsset asset;
                asset.guid = guid;
                asset.type = stringToAssetType(obj["type"]);
                asset.name = obj.value("name", "");
                asset.main_relative_path = std::filesystem::path(obj.value("main_relative_path", ""));
                asset.shipped_relative_path = std::filesystem::path(obj.value("shipped_relative_path", ""));
                assets[guid] = std::make_shared<IAsset>(asset);
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
                else if (dxgiFormat == 95) { // DXGI_FORMAT_BC6H_UF16
                    tex->format = TextureFormat::BC6H;
                    tex->glTexFormat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
                }
                else if (dxgiFormat == 96) { // DXGI_FORMAT_BC6H_SF16
                    tex->format = TextureFormat::BC6H;
                    tex->glTexFormat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
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

                if (glfwGetCurrentContext() == nullptr) {
                    PN_CORE_ERROR("No current OpenGL context - cannot upload texture!");
                    throw std::runtime_error("No current OpenGL context.");
                }

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

            //Check if virtual path has valid extension
            if (std::filesystem::path(path_service->resolvePath(virtual_path)).extension() != ".mesh") {
                throw std::runtime_error("Invalid file type: " + virtual_path);
            }

            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good()) {
                PN_CORE_ERROR("Failed to open model file: {}", virtual_path);
                throw std::runtime_error("Failed to open model file: " + virtual_path);
            }

            std::vector<uint8_t> data(stream->size());
            size_t read = stream->read(data.data(), data.size());
            if (read != data.size()) {
                PN_CORE_ERROR("Failed to read full model file: {}", virtual_path);
                throw std::runtime_error("Failed to read full model file: " + virtual_path);
            }

            Model asset;
            size_t offset = 0;
            auto require = [&](size_t n) {
                if (offset + n > data.size()) { 
                    PN_CORE_ERROR("Unexpected end of model file");
                    throw std::runtime_error("Unexpected end of model file"); 
                }
                };
            auto readMem = [&](void* dst, size_t n) {
                if (n == 0) return;
                require(n); std::memcpy(dst, data.data() + offset, n); offset += n;
                };

            PN_CORE_INFO("File size: {} bytes", data.size());

            PN_CORE_TRACE("ImportModel: Before reading bounding box");

            // Read bounding box
            PN_CORE_TRACE("sizeof(glm::vec3) = {}", sizeof(glm::vec3));
            PN_CORE_TRACE("Offset before AABB: {}", offset);
            readMem(&asset.aabbMin, sizeof(asset.aabbMin));
            PN_CORE_TRACE("Offset after aabbMin: {}", offset);
            readMem(&asset.aabbMax, sizeof(asset.aabbMax));
            PN_CORE_TRACE("Offset after aabbMax: {}", offset);

            PN_CORE_TRACE("ImportModel: Before reading LODs");

            // Read LODs
            uint32_t lodCount = 0;
            readMem(&lodCount, sizeof(lodCount));
            PN_CORE_TRACE("lodCount = {} at offset {}", lodCount, offset - 4);
            PN_CORE_TRACE("ImportModel: after readMem for LOD");
            PN_CORE_TRACE("Attempting to allocate {} bytes for `asset`", lodCount);
            asset.lods.resize(lodCount);
            PN_CORE_TRACE("ImportModel: after reszing asset for LOD");
            readMem(asset.lods.data(), lodCount * sizeof(uint32_t));

            PN_CORE_TRACE("ImportModel: Before reading vertices");

            // Vertices/Indices
            uint32_t vtxCount = 0, idxCount = 0;
            readMem(&vtxCount, sizeof(vtxCount));
            readMem(&idxCount, sizeof(idxCount));
            asset.vertices.resize(vtxCount);
            asset.indices.resize(idxCount);
            readMem(asset.vertices.data(), vtxCount * sizeof(Vertex));
            readMem(asset.indices.data(), idxCount * sizeof(uint32_t));

            PN_CORE_TRACE("ImportModel: Before reading submeshes");

            // Submeshes
            uint32_t submeshCount = 0;
            readMem(&submeshCount, sizeof(submeshCount));
            asset.submeshes.resize(submeshCount);
            for (Submesh& sm : asset.submeshes) {
                uint32_t nameLen = 0;
                readMem(&nameLen, sizeof(nameLen));
                sm.name.resize(nameLen);
                readMem(sm.name.data(), nameLen);
                readMem(&sm.materialIndex, sizeof(sm.materialIndex));
                readMem(&sm.firstIndex, sizeof(sm.firstIndex));
                readMem(&sm.indexCount, sizeof(sm.indexCount));
                readMem(&sm.vertexOffset, sizeof(sm.vertexOffset));
            }

            // MorphTargets
            uint32_t morphCount = 0;
            readMem(&morphCount, sizeof(morphCount));
            asset.morphTargets.resize(morphCount);
            for (MorphTarget& mt : asset.morphTargets) {
                uint32_t nameLen = 0, deltaCount = 0;
                readMem(&nameLen, sizeof(nameLen));
                mt.name.resize(nameLen);
                readMem(mt.name.data(), nameLen);

                readMem(&deltaCount, sizeof(deltaCount));
                mt.positionDeltas.resize(deltaCount);
                mt.normalDeltas.resize(deltaCount);
                readMem(mt.positionDeltas.data(), deltaCount * sizeof(glm::vec3));
                readMem(mt.normalDeltas.data(), deltaCount * sizeof(glm::vec3));
            }

            // Skeleton Bones
            uint32_t boneCount = 0;
            readMem(&boneCount, sizeof(boneCount));
            asset.skeleton.resize(boneCount);
            for (Bone& b : asset.skeleton) {
                uint32_t nameLen = 0;
                readMem(&nameLen, sizeof(nameLen));
                b.name.resize(nameLen);
                readMem(b.name.data(), nameLen);
                readMem(&b.parent, sizeof(b.parent));
                readMem(&b.bindPose, sizeof(glm::mat4));
            }

            // Animations
            uint32_t animCount = 0;
            readMem(&animCount, sizeof(animCount));
            asset.animations.resize(animCount);
            for (AnimationClip& anim : asset.animations) {
                uint32_t nameLen = 0;
                readMem(&nameLen, sizeof(nameLen));
                anim.name.resize(nameLen);
                readMem(anim.name.data(), nameLen);
                readMem(&anim.duration, sizeof(anim.duration));
                readMem(&anim.isAdditive, sizeof(anim.isAdditive));

                uint32_t trackCount = 0;
                readMem(&trackCount, sizeof(trackCount));
                anim.tracks.resize(trackCount);
                for (AnimationTrack& track : anim.tracks) {
                    uint32_t boneLen = 0, keyCount = 0;
                    readMem(&boneLen, sizeof(boneLen));
                    track.boneName.resize(boneLen);
                    readMem(track.boneName.data(), boneLen);

                    readMem(&keyCount, sizeof(keyCount));
                    track.keys.resize(keyCount);
                    for (AnimationKey& key : track.keys) {
                        readMem(&key.time, sizeof(key.time));
                        readMem(&key.translation, sizeof(key.translation));
                        readMem(&key.rotation, sizeof(key.rotation));
                        readMem(&key.scale, sizeof(key.scale));
                        // Morph weights
                        uint32_t morphWeightsCount = 0;
                        readMem(&morphWeightsCount, sizeof(morphWeightsCount));
                        key.morphTargetWeights.resize(morphWeightsCount);
                        readMem(key.morphTargetWeights.data(), morphWeightsCount * sizeof(float));
                    }
                }
            }

            PN_CORE_TRACE("ImportModel: Before reading materials");

            // Materials
            uint32_t matCount = 0;
            readMem(&matCount, sizeof(matCount));
            asset.materials.resize(matCount);
            for (Material& mat : asset.materials) {
                uint32_t nameLen = 0;
                readMem(&nameLen, sizeof(nameLen));
                mat.name.resize(nameLen);
                readMem(mat.name.data(), nameLen);

                auto readStr = [&](std::string& str) {
                    uint32_t len = 0;
                    readMem(&len, sizeof(len));
                    str.resize(len);
                    readMem(str.data(), len);
                    };

                readStr(mat.diffuse_map_buf);
                readStr(mat.normalMap);
                readStr(mat.metallicMap);
                readStr(mat.roughnessMap);
                readStr(mat.aoMap);
                readStr(mat.emissionMap);

                readMem(&mat.baseColor, sizeof(mat.baseColor));
                readMem(&mat.metallic, sizeof(mat.metallic));
                readMem(&mat.roughness, sizeof(mat.roughness));
                readMem(&mat.emission, sizeof(mat.emission));
            }

            asset.vpath = virtual_path;

            return std::make_shared<Model>(std::move(asset));
		}

        bool Loader::CheckShader(GLuint shader, const char* label) const {
            GLint compiled = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled == GL_TRUE) return true;

            GLint len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            std::string log(len ? len - 1 : 0, '\0');
            if (len > 1) glGetShaderInfoLog(shader, len, nullptr, log.data());
#ifdef PN_PLATFORM_ANDROID
            PN_CORE_ERROR("[Shader] Compile failed ({0}):\n{1}", label, log);
#else
            PN_CORE_ERROR("[Shader] Compile failed ({0}):\n{1}", label, log);
#endif
            return false;
        }

        bool Loader::CheckProgram(GLuint program) const {
            GLint linked = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linked);
            if (linked == GL_TRUE) return true;

            GLint len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
            std::string log(len ? len - 1 : 0, '\0');
            if (len > 1) glGetProgramInfoLog(program, len, nullptr, log.data());
#ifdef PN_PLATFORM_ANDROID
            PN_CORE_ERROR("[Shader] Link failed:\n{0}", log);
#else
            PN_CORE_ERROR("[Shader] Link failed:\n{0}", log);
#endif
            return false;
        }

        uint32_t Loader::CompileShader(unsigned int type, const std::string& source) const
        {
            // Create vert & frag shaders
            uint32_t shader = glCreateShader(type);
            const char* src = source.c_str();

            PN_CORE_INFO("Compiling {} shader", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
            // PN_CORE_INFO("Shader source:\n{0}", source);

            glShaderSource(shader, 1, &src, nullptr);
            glCompileShader(shader);

            // Check compilation
            int success;

            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                char infoLog[512];
                glGetShaderInfoLog(shader, 512, nullptr, infoLog);
#ifdef PN_PLATFORM_ANDROID
                PN_CORE_ERROR("Shader compile error {0}: {1}\nSource: {2}",
                    type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog, source);
#else
                PN_CORE_ERROR("Shader Compilation Failed ({0}): {1}", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog);
#endif

                assert(false);
            }

            return shader;

        }

        uint32_t Loader::LinkProgram(unsigned int vert_shader, unsigned int frag_shader) const
        {
            GLuint program = glCreateProgram();
            glAttachShader(program, vert_shader);
            glAttachShader(program, frag_shader);
            glLinkProgram(program);

            GLint numUniforms = 0;
            glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
            PN_CORE_INFO("Linked program {} has {} active uniforms", program, numUniforms);

            for (int i = 0; i < numUniforms; i++) {
                char name[256] = { 0 };
                GLsizei length = 0;
                GLint size = 0;
                GLenum type = 0;
                glGetActiveUniform(program, i, 256, &length, &size, &type, name);
                PN_CORE_INFO("  Uniform {}: '{}'", i, name);
            }

            if (!CheckProgram(program)) {
#ifdef PN_PLATFORM_ANDROID
                PN_CORE_ERROR("Program link FAILED");
#else
                PN_CORE_ERROR("Program link FAILED");
#endif
                assert("Program link failed");
            }
            else {
                PN_CORE_INFO("Program link succeeded");
            }

            // (Optional but recommended)
            glDetachShader(program, vert_shader);
            glDetachShader(program, frag_shader);

            return program;
        }

        std::shared_ptr<Shader> Loader::ImportShader(std::string const& virtual_vert, std::string const& virtual_frag) const {

            //Get path service
            auto path_service = services->get<Path::Path>();

            //Read data
            PN_CORE_INFO("Using paths: {0}, {1}", virtual_vert, virtual_frag);
            auto vert_stream = path_service->createFileStream(virtual_vert, Path::FileMode::Read);
            std::string vert_code(vert_stream->size(), '\0');
            size_t chunk = vert_stream->read(&vert_code[0], vert_code.size());
            PN_CORE_INFO("Successfully read vertex shader");
            auto frag_stream = path_service->createFileStream(virtual_frag, Path::FileMode::Read);
            std::string frag_code(frag_stream->size(), '\0');
            chunk = frag_stream->read(&frag_code[0], frag_code.size());
            PN_CORE_INFO("Successfully read fragment shader");

            //Compiler shader
            uint32_t vert_shader = CompileShader(GL_VERTEX_SHADER, vert_code);
            uint32_t frag_shader = CompileShader(GL_FRAGMENT_SHADER, frag_code);

            //Link program
            std::shared_ptr<Shader> shader = std::make_shared<Shader>(LinkProgram(vert_shader, frag_shader));

            // Clean up shaders (they're linked now)
            glDeleteShader(vert_shader);
            glDeleteShader(frag_shader);

            return shader;
        }

        std::shared_ptr<Fonts::FontFace> Loader::ImportFont(std::string const& virtual_path) const {
            return std::make_shared<Fonts::FontFace>(path_service, virtual_path);
        }
	}
}