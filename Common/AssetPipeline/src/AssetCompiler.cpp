#include "AssetCompiler.h"

#ifdef PN_PLATFORM_WINDOWS
#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace PAIN {
	namespace Assets {

        uint64_t Compiler::getCurrentTimeStamp() const {
            //Get current timestamp in milliseconds since epoch
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto current_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            return current_timestamp;
        }

        bool Compiler::verifyDirectory(std::filesystem::path const& dest) const {

            //Check if directory exists
            if (!std::filesystem::exists(dest)) {

                //Create directory if it doesnt exist
                std::filesystem::path parent_dir = dest.parent_path();
                if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
                    if (!std::filesystem::create_directories(parent_dir)) {
                        std::cout << "Failed to create parent directory: " << parent_dir << std::endl;
                        return false;
                    }

                    std::cout << "Created directory: " << parent_dir << std::endl;
                    return true;
                }
            }

            return true;
        }

		bool Compiler::copyFile(std::filesystem::path const& copy, std::filesystem::path const& dest) const {
            try {

                //Verify directory
                if (!verifyDirectory(dest)) {
                    std::cout << "Directory doesnt exist: " << dest << std::endl;
                    return false;
                }

                //Use std::filesystem::copy_file with update_existing option
                std::filesystem::copy_options options =
                    std::filesystem::copy_options::update_existing;

                if (std::filesystem::copy_file(copy, dest, options)) {
                    std::cout << "File Copied From: " << copy << " To: " << dest << std::endl;
                    return true;
                }
                else {
                    std::cout << copy << " Copy Failed." << std::endl;
                    return false;
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cout << copy << " Copy Failed." << e.what() << std::endl;
                return false;
            }
		}

        nlohmann::json Compiler::generateDefaultCompileSettings(Type const& type, Info const& asset) const {
            nlohmann::json settings;

            switch (type) {
            case Type::Texture: {
                settings["window_compression"] = "BC7";
                settings["android_compression"] = "ASTC_4x4";
                settings["generate_mipmaps"] = true;
                settings["max_size"] = 1024;
                settings["srgb"] = true;
                break;
            }
            case Type::Audio: {

                settings["compression"] = "OGG";
                settings["extension"] = ".ogg";
                settings["quality"] = 0.8;

                bool isMusic = false;

                //For determining if its music
                if (std::filesystem::exists(asset.raw_path)) {
                    uintmax_t fileSize = std::filesystem::file_size(asset.raw_path);
                    constexpr uintmax_t MUSIC_FILESIZE_THRESHOLD = 1 * 1024 * 1024; // 1MB
                    isMusic = fileSize > MUSIC_FILESIZE_THRESHOLD;
                }
               isMusic = (isMusic || asset.raw_path.string().find("music") != std::string::npos ||
                    asset.raw_path.string().find("bgm") != std::string::npos);

                if (isMusic) {
                    settings["loop"] = true;
                }
                else {
                    settings["loop"] = false;
                }
                break;
            }
            case Type::Model: {
                settings["extension"] = ".mesh";
                settings["optimize_vertices"] = true;
                settings["weld_threshold"] = 0.001f;
                settings["generate_normals"] = false;  // Only if missing
                settings["generate_tangents"] = true;  // For normal mapping
                settings["triangulate"] = true;
                settings["join_identical_vertices"] = true;
                settings["remove_redundant_materials"] = true;
                settings["optimize_meshes"] = true;
                settings["improve_cache_locality"] = true;

                // Compression/quantization
                settings["quantize_positions"] = false;  // Enable for mobile
                settings["quantize_normals"] = false;
                settings["quantize_uvs"] = false;

                // LOD generation (advanced)
                settings["generate_lods"] = false;
                settings["lod_levels"] = 3;
                settings["lod_reduction"] = 0.5f;  // 50% reduction per level

                // Animation/skeleton
                settings["import_animations"] = true;
                settings["import_skeleton"] = true;
                settings["max_bone_weights"] = 4;
                break;
            }
            default:
                break;
            }

            return settings;
        }

        bool Compiler::verifyCompileSettings(Type const& type, nlohmann::json const& settings) const {

            try {

                bool checker = true;

                switch (type) {
                case Type::Texture:
                    checker =   (settings.contains("window_compression") &&
                                settings.contains("android_compression") &&
                                settings.contains("generate_mipmaps") &&
                                settings.contains("max_size") &&
                                settings.contains("srgb"));
                    break;

                case Type::Audio:
                    checker =   (settings.contains("compression") &&
                                settings.contains("extension") &&
                                settings.contains("quality") &&
                                settings.contains("loop"));
                    break;

                case Type::Model:
                    checker = (settings.contains("extension") &&
                        settings.contains("optimize_vertices") &&
                        settings.contains("weld_threshold") &&
                        settings.contains("generate_normals") &&
                        settings.contains("generate_tangents") &&
                        settings.contains("triangulate") &&
                        settings.contains("join_identical_vertices") &&
                        settings.contains("remove_redundant_materials") &&
                        settings.contains("optimize_meshes") &&
                        settings.contains("improve_cache_locality") &&
                        settings.contains("quantize_positions") &&
                        settings.contains("quantize_normals") &&
                        settings.contains("quantize_uvs") &&
                        settings.contains("generate_lods") &&
                        settings.contains("lod_levels") &&
                        settings.contains("lod_reduction") &&
                        settings.contains("import_animations") &&
                        settings.contains("import_skeleton") &&
                        settings.contains("max_bone_weights"));
                    break;

                default:
                    break;
                }

                if (!checker) return false;
                return true;
            }
            catch (const std::exception& e) {
                return false;
            }

        }

        Descriptor Compiler::createDefaultDesc(Info& asset, std::filesystem::path const& path) const {

            //Extract asset name from path
            std::string asset_name = asset.raw_path.filename().string();

            //Create default descriptor
            Descriptor desc;

            //Identity
            if (asset.guid.IsValid()) {
                desc.guid = asset.guid;
            }
            else {
                desc.guid = GUID::Generate();
                asset.guid = desc.guid;
            }
            desc.descriptor_version = 1;

            //Asset classification
            desc.type = asset.type;
            desc.name = asset_name;

            //Import/processing settings
            desc.import_settings = generateDefaultCompileSettings(desc.type, asset);

            //Build data
            desc.hash = fileHashing(asset.raw_path);

            //Dependencies
            desc.dependencies.clear();

            //Metadata (start empty, expandable)
            desc.meta_data = nlohmann::json::object();

            //Add some basic metadata based on asset type
            desc.meta_data["source_file"] = asset.relative_folder / asset.name;

            //Save desc file
            saveDescFile(desc, path);

            return desc;
        }

        Descriptor Compiler::readDescFile(Info& asset, std::filesystem::path const& path) const {

            bool needs_updating = false;

            try {
                std::ifstream file(path);
                nlohmann::json desc_json;
                file >> desc_json;

                Descriptor desc;
                desc.descriptor_version = desc_json.value("descriptor_version", 1);
                desc.guid = GUID(desc_json["guid"].get<std::string>());
                asset.guid = desc.guid;

                //Asset info
                auto asset_info = desc_json["asset_info"];
                desc.type = stringToAssetType(asset_info["type"].get<std::string>());
                desc.name = asset_info.value("name", asset.name);

                //Settings and build data
                desc.import_settings = desc_json.value("import_settings", nlohmann::json{});
                auto build_data = desc_json["build_data"];
                desc.hash = build_data.value("hash", std::size_t(0));

                //Verify import settings
                if (!verifyCompileSettings(asset.type, desc.import_settings)) {
                    desc.import_settings = generateDefaultCompileSettings(desc.type, asset);
                    needs_updating = true;
                }

                //Dependencies
                auto deps_array = desc_json.value("dependencies", nlohmann::json::array());
                for (const auto& dep_str : deps_array) {
                    desc.dependencies.push_back(GUID(dep_str.get<std::string>()));
                }

                desc.meta_data = desc_json.value("meta_data", nlohmann::json{});

                // Verify desc file source
                std::filesystem::path expected_source = asset.relative_folder / asset.name;
                std::filesystem::path current_source = desc.meta_data.value("source_file", "");

                if (current_source != expected_source) {
                    desc.meta_data["source_file"] = expected_source.string();
                    needs_updating = true;
                }

                file.close();

                //if file needs to be updated.
                if (needs_updating) {
                    saveDescFile(desc, path);
                }

                return desc;
            }
            catch (const std::exception& e) {
                std::cout << "Error encountered reading desc file, reverting to default." << std::endl;
                return createDefaultDesc(asset, path);
            }
        }

        Descriptor Compiler::readDescFile(std::filesystem::path const& path) const {

            Descriptor desc;

            try {
                std::ifstream file(path);
                nlohmann::json desc_json;
                file >> desc_json;

                desc.descriptor_version = desc_json.value("descriptor_version", 1);
                desc.guid = GUID(desc_json["guid"].get<std::string>());

                //Asset info
                auto asset_info = desc_json["asset_info"];
                desc.type = stringToAssetType(asset_info["type"].get<std::string>());
                desc.name = asset_info.value("name", "");

                //Settings and build data
                desc.import_settings = desc_json.value("import_settings", nlohmann::json{});
                auto build_data = desc_json["build_data"];
                desc.hash = build_data.value("hash", std::size_t(0));

                //Dependencies
                auto deps_array = desc_json.value("dependencies", nlohmann::json::array());
                for (const auto& dep_str : deps_array) {
                    desc.dependencies.push_back(GUID(dep_str.get<std::string>()));
                }

                desc.meta_data = desc_json.value("meta_data", nlohmann::json{});
                desc.meta_data["source_file"] = desc.meta_data.value("source_file", "");

                file.close();

                return desc;
            }
            catch (const std::exception& e) {
                std::cout << "Error encountered reading desc file, some information might be invalid." << std::endl;
                return desc;
            }
        }

        bool Compiler::saveDescFile(Descriptor const& desc_file, std::filesystem::path const& path) const {
            //Save generated descriptor
            try {
                //Verify directory
                if (!verifyDirectory(path)) {
                    std::cout << "Directory doesnt exist: " << path << std::endl;
                    return false;
                }

                nlohmann::json desc_json;

                //Core identity
                desc_json["descriptor_version"] = desc_file.descriptor_version;
                desc_json["guid"] = desc_file.guid.ToString();

                //Asset info
                desc_json["asset_info"]["type"] = assetTypeToString(desc_file.type);
                desc_json["asset_info"]["name"] = desc_file.name;

                //Settings and build data
                desc_json["import_settings"] = desc_file.import_settings;
                desc_json["build_data"]["hash"] = desc_file.hash;

                //Dependencies and metadata
                nlohmann::json deps_array = nlohmann::json::array();
                for (const auto& dep : desc_file.dependencies) {
                    deps_array.push_back(dep.ToString());
                }
                desc_json["dependencies"] = deps_array;
                desc_json["meta_data"] = desc_file.meta_data;

                std::ofstream file(path, std::ios::out);
                if (file << desc_json.dump(2)) {
                    std::cout << "Descriptor file saved at: " << path << std::endl;
                    file.close();
                    return true;
                }
                std::cout << "Error saving default desc file to: " << path << std::endl;
                file.close();
                return false;
            }
            catch (const std::exception& e) {
                std::cout << "Error saving default desc file to: " << path << std::endl;
                return false;
            }
        }

        void Compiler::compileAndShip(Descriptor& desc_file, Info& asset_info) const {

            //Find asset type and platform
            switch (desc_file.type) {
            case Type::Texture:
                compileTexture(desc_file, asset_info);
                break;

            case Type::Audio:
                compileAudio(desc_file, asset_info);
                break;

            case Type::Model:
                compileModel(desc_file, asset_info);
                break;
                
            default:
                break;
            }
        }

        void Compiler::compileTexture(Descriptor& desc_file, Info& asset_info) const {

            //Determine output format and shipped path
            std::string output_extension;
            std::string compression_format;

            switch (platform) {
            case Platform::Windows:
                output_extension = ".dds";
                compression_format = desc_file.import_settings.value("compression", "BC7");
                asset_info.shipped_path = output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + output_extension);
                break;
            case Platform::Android:
                output_extension = ".astc";
                compression_format = desc_file.import_settings.value("compression", "ASTC_4x4");
                asset_info.shipped_path = output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + output_extension);
                break;
            default:
                std::cout << "ERROR: Unsupported platform for texture compilation" << std::endl;
                return;
            }

            //Check if recompilation is needed
            if (!needsRecompilation(asset_info, desc_file)) return;

            //Load texture data using STB
            stbi_set_flip_vertically_on_load(false);
            int width, height, channels;
            unsigned char* raw_pixels = stbi_load(asset_info.raw_path.string().c_str(),
                &width, &height, &channels, STBI_rgb_alpha);

            if (!raw_pixels) {
                std::cout << "ERROR: Failed to load texture: " << asset_info.raw_path
                    << " - " << stbi_failure_reason() << std::endl;
                return;
            }

            std::cout << "Loaded texture: " << width << "x" << height << " (" << channels << " channels)" << std::endl;

            //Apply import settings (resize if needed)
            int target_width = width;
            int target_height = height;
            int max_size = desc_file.import_settings.value("max_size", 2048);

            if (width > max_size || height > max_size) {
                float scale = static_cast<float>(max_size) / std::max(width, height);
                int target_width = static_cast<int>(width * scale);
                int target_height = static_cast<int>(height * scale);

                //Resize using STB
                unsigned char* resized_pixels = (unsigned char*)malloc(target_width * target_height * 4);

                // NEW API: stbir_resize (not stbir_resize_uint8)
                if (stbir_resize(raw_pixels, width, height, 0,           // input
                    resized_pixels, target_width, target_height, 0, // output  
                    STBIR_RGBA, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT)) { 

                    stbi_image_free(raw_pixels);
                    raw_pixels = resized_pixels;
                    width = target_width;
                    height = target_height;
                    std::cout << "Resized texture to: " << width << "x" << height << std::endl;
                }
                else {
                    std::cout << "ERROR: STB resize failed!" << std::endl;
                    free(resized_pixels);
                }
            }

            //Verify output directory
            if (!verifyDirectory(asset_info.shipped_path)) {
                std::cout << "ERROR: Failed to create output directory: " << asset_info.shipped_path.parent_path() << std::endl;
                stbi_image_free(raw_pixels);
                return;
            }

            //Compress assets
            bool compression_success = false;

            if (platform == Platform::Windows) {
                //Use Cuttlefish for DDS/BC7 compression
                compression_success = CompressTextureDDS(raw_pixels, width, height, 4,
                    asset_info.shipped_path.string(),
                    compression_format, desc_file.import_settings);
            }
            else if (platform == Platform::Android) {
                //Use astc encoder for ASTC compression
                compression_success = CompressTextureASTC(raw_pixels, width, height, 4,
                    asset_info.shipped_path.string(),
                    compression_format, desc_file.import_settings);
            }

            //Clean up
            stbi_image_free(raw_pixels);

            //Verify output
            if (compression_success && std::filesystem::exists(asset_info.shipped_path)) {
                std::cout << "Texture compiled successfully: " << asset_info.shipped_path.filename() << std::endl;

                //Update desc file with hashing
                desc_file.hash = fileHashing(asset_info.raw_path);
            }
            else {
                std::cout << "ERROR: Texture compilation failed for: " << asset_info.raw_path.filename() << std::endl;
                asset_info.shipped_path.extension().replace_extension(asset_info.raw_path.extension());
                copyFile(asset_info.raw_path, asset_info.shipped_path);
            }
        }

        void Compiler::compileAudio(Descriptor& desc_file, Info& asset_info) const {

            //Set output directory
            std::string out_ext = desc_file.import_settings.value("extension", ".ogg"); // fallback to 0.8 if not specified
            asset_info.shipped_path = output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + out_ext);

            //Check if recompilation is needed
            if (!needsRecompilation(asset_info, desc_file)) return;

            // Paths
            std::string ffmpegExe = GetFFMPEGExecutable();
            std::string inputPath = asset_info.raw_path.string(); // Your input PCM/WAV/AIFF/etc.
            std::string outputPath = asset_info.shipped_path.string(); // Destination .ogg file

            // Import settings
            float quality = desc_file.import_settings.value("quality", 0.8f); // fallback to 0.8 if not specified
            bool shouldLoop = desc_file.import_settings.value("loop", false);

            std::filesystem::create_directories(std::filesystem::path(outputPath).parent_path());

            // Build ffmpeg command for OGG conversion
            std::stringstream cmd;
            cmd << "cmd /C \""
                << "\"" << ffmpegExe << "\""
                << " -y"
                << " -i \"" << inputPath << "\""
                << " -c:a libvorbis"
                << " -qscale:a " << quality * 10
                << " \"" << outputPath << "\""
                << "\""; // Close the CMD string

            // Launch ffmpeg as a process
            int result = std::system(cmd.str().c_str());
            if (result != 0) {
                std::cerr << "[ERROR] FFmpeg conversion failed: " << cmd.str() << std::endl;
                asset_info.shipped_path.extension().replace_extension(asset_info.raw_path.extension());
                copyFile(asset_info.raw_path, asset_info.shipped_path);
            }
            else {
                std::cout << "[Audio] Compiled " << inputPath << " to OGG at " << outputPath << std::endl;

                //Update desc file with hashing
                desc_file.hash = fileHashing(asset_info.raw_path);
            }
        }

        void Compiler::compileModel(Descriptor& desc_file, Info& asset_info) const {

            // Set output extension
            std::string out_ext = desc_file.import_settings.value("extension", ".mesh");
            asset_info.shipped_path = output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + out_ext);

            // Check if recompilation is needed
            if (!needsRecompilation(asset_info, desc_file)) return;

            // Get import settings
            bool optimize_vertices = desc_file.import_settings.value("optimize_vertices", true);
            float weld_threshold = desc_file.import_settings.value("weld_threshold", 0.001f);
            bool generate_tangents = desc_file.import_settings.value("generate_tangents", true);
            bool triangulate = desc_file.import_settings.value("triangulate", true);
            bool import_animations = desc_file.import_settings.value("import_animations", true);
            bool import_skeleton = desc_file.import_settings.value("import_skeleton", true);
            int max_bone_weights = desc_file.import_settings.value("max_bone_weights", 4);

            // Configure Assimp post-processing flags
            unsigned int ppFlags = 0;
            if (triangulate) ppFlags |= aiProcess_Triangulate;
            if (optimize_vertices) ppFlags |= aiProcess_JoinIdenticalVertices;
            if (desc_file.import_settings.value("optimize_meshes", true)) ppFlags |= aiProcess_OptimizeMeshes;
            if (desc_file.import_settings.value("improve_cache_locality", true)) ppFlags |= aiProcess_ImproveCacheLocality;
            if (desc_file.import_settings.value("remove_redundant_materials", true)) ppFlags |= aiProcess_RemoveRedundantMaterials;
            if (generate_tangents) ppFlags |= aiProcess_CalcTangentSpace;
            if (desc_file.import_settings.value("generate_normals", false)) ppFlags |= aiProcess_GenNormals;

            ppFlags |= aiProcess_GenSmoothNormals;
            ppFlags |= aiProcess_FlipUVs;

            // Load with Assimp
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(asset_info.raw_path.string(), ppFlags);

            if (!scene || !scene->HasMeshes()) {
                std::cerr << "Failed to load model: " << importer.GetErrorString() << std::endl;
                return;
            }

            Model asset;

            // First, extract bone names and bind poses (for skeleton)
            std::unordered_map<std::string, int> boneNameToIndex;
            for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
                aiMesh* mesh = scene->mMeshes[m];
                for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
                    aiBone* bone = mesh->mBones[b];
                    std::string boneName = bone->mName.C_Str();
                    if (boneNameToIndex.find(boneName) == boneNameToIndex.end()) {
                        Bone joint;
                        joint.name = boneName;
                        joint.parent = -1; // Optionally resolve parent below if needed
                        aiMatrix4x4 m = bone->mOffsetMatrix;
                        joint.bindPose = glm::mat4(
                            m.a1, m.b1, m.c1, m.d1,
                            m.a2, m.b2, m.c2, m.d2,
                            m.a3, m.b3, m.c3, m.d3,
                            m.a4, m.b4, m.c4, m.d4
                        );
                        boneNameToIndex[boneName] = static_cast<int>(asset.skeleton.size());
                        asset.skeleton.push_back(joint);
                    }
                }
            }

            size_t vertexBase = 0;
            for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
                aiMesh* mesh = scene->mMeshes[m];

                size_t firstVertex = asset.vertices.size();
                size_t firstIndex = asset.indices.size();

                // Vertex extraction
                for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                    Vertex vert;
                    vert.pos = glm::vec3(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
                    vert.normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[v].x, mesh->mNormals[v].y, mesh->mNormals[v].z) : glm::vec3(0, 0, 1);
                    vert.uv = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y) : glm::vec2(0);
                    vert.tangent = mesh->HasTangentsAndBitangents()
                        ? glm::vec3(mesh->mTangents[v].x, mesh->mTangents[v].y, mesh->mTangents[v].z)
                        : glm::vec3(1, 0, 0);
                    vert.bitangent = mesh->HasTangentsAndBitangents()
                        ? glm::vec3(mesh->mBitangents[v].x, mesh->mBitangents[v].y, mesh->mBitangents[v].z)
                        : glm::vec3(0, 1, 0);
                    vert.color = mesh->HasVertexColors(0)
                        ? glm::vec3(mesh->mColors[0][v].r, mesh->mColors[0][v].g, mesh->mColors[0][v].b)
                        : glm::vec3(1, 1, 1);

                    // Bone assignment: collect up to 4, sorted greatest weight first
                    std::vector<std::pair<uint32_t, float>> weights;
                    // For all bones in mesh, check if this vertex is influenced
                    if (mesh->HasBones()) {
                        for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
                            aiBone* bone = mesh->mBones[b];
                            for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                                if (mesh->mBones[b]->mWeights[w].mVertexId == v) {
                                    weights.push_back({ boneNameToIndex[bone->mName.C_Str()], bone->mWeights[w].mWeight });
                                }
                            }
                        }
                    }
                    // Sort and clamp to max_bone_weights
                    std::sort(weights.begin(), weights.end(),
                        [](const auto& a, const auto& b) { return a.second > b.second; });
                    while (weights.size() < max_bone_weights) weights.push_back({ 0,0.0f });
                    if (weights.size() > max_bone_weights) weights.resize(max_bone_weights);

                    for (int i = 0; i < max_bone_weights; ++i) {
                        vert.boneIndices[i] = (uint8_t)weights[i].first;
                        vert.boneWeights[i] = weights[i].second;
                    }

                    asset.vertices.push_back(vert);
                }

                // Index extraction
                for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
                    for (unsigned int i = 0; i < mesh->mFaces[f].mNumIndices; ++i) {
                        asset.indices.push_back(static_cast<unsigned int>(firstVertex + mesh->mFaces[f].mIndices[i]));
                    }
                }

                // Submesh extraction
                Submesh submesh;
                submesh.name = mesh->mName.C_Str();
                submesh.firstIndex = (uint32_t)firstIndex;
                submesh.indexCount = (uint32_t)(asset.indices.size() - firstIndex);
                submesh.vertexOffset = (uint32_t)firstVertex;
                submesh.materialIndex = mesh->mMaterialIndex;
                asset.submeshes.push_back(submesh);

                vertexBase += mesh->mNumVertices;

                // --- Morph Target Extraction (Blend Shapes) ---
                for (unsigned int mt = 0; mt < mesh->mNumAnimMeshes; ++mt) {
                    aiAnimMesh* animMesh = mesh->mAnimMeshes[mt];
                    MorphTarget morph;
                    morph.name = animMesh->mName.C_Str();

                    uint32_t deltaCount = mesh->mNumVertices;
                    morph.positionDeltas.resize(deltaCount);
                    morph.normalDeltas.resize(deltaCount);

                    for (uint32_t v = 0; v < deltaCount; ++v) {
                        if (animMesh->mVertices) {
                            // delta = morph - base
                            morph.positionDeltas[v] = glm::vec3(animMesh->mVertices[v].x - mesh->mVertices[v].x,
                                animMesh->mVertices[v].y - mesh->mVertices[v].y,
                                animMesh->mVertices[v].z - mesh->mVertices[v].z);
                        }
                        if (animMesh->mNormals && mesh->mNormals) {
                            morph.normalDeltas[v] = glm::vec3(animMesh->mNormals[v].x - mesh->mNormals[v].x,
                                animMesh->mNormals[v].y - mesh->mNormals[v].y,
                                animMesh->mNormals[v].z - mesh->mNormals[v].z);
                        }
                    }
                    asset.morphTargets.push_back(morph);
                }
                // -----------------------------------------------
            }

            // Animation extraction as previously shown (unchanged)

            if (import_animations && scene->HasAnimations()) {
                for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
                    aiAnimation* anim = scene->mAnimations[a];
                    AnimationClip clip;
                    clip.name = anim->mName.C_Str();
                    clip.duration = static_cast<float>(anim->mDuration) / static_cast<float>(anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0f);
                    for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
                        aiNodeAnim* chan = anim->mChannels[c];
                        AnimationTrack track;
                        track.boneName = chan->mNodeName.C_Str();
                        for (unsigned int k = 0; k < chan->mNumPositionKeys; ++k) {
                            AnimationKey key;
                            key.time = static_cast<float>(chan->mPositionKeys[k].mTime) / static_cast<float>(anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0f);
                            key.translation = glm::vec3(chan->mPositionKeys[k].mValue.x, chan->mPositionKeys[k].mValue.y, chan->mPositionKeys[k].mValue.z);
                            if (k < chan->mNumRotationKeys) {
                                key.rotation = glm::quat(chan->mRotationKeys[k].mValue.w,
                                    chan->mRotationKeys[k].mValue.x,
                                    chan->mRotationKeys[k].mValue.y,
                                    chan->mRotationKeys[k].mValue.z
                                );
                            }
                            if (k < chan->mNumScalingKeys) {
                                key.scale = glm::vec3(chan->mScalingKeys[k].mValue.x, chan->mScalingKeys[k].mValue.y, chan->mScalingKeys[k].mValue.z);
                            }
                            // Morph target weights can be set here if you support animated blend shapes
                            track.keys.push_back(key);
                        }
                        clip.tracks.push_back(track);
                    }
                    asset.animations.push_back(clip);
                }
            }

            for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {
                aiMaterial* material = scene->mMaterials[m];
                Material mat;
                mat.name = material->GetName().C_Str();
                aiString texPath;

                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
                    mat.diffuseMap = texPath.C_Str();
                if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS)
                    mat.normalMap = texPath.C_Str();
                if (material->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS)
                    mat.metallicMap = texPath.C_Str();
                if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texPath) == AI_SUCCESS)
                    mat.roughnessMap = texPath.C_Str();

                // Base (albedo) color
                aiColor3D baseColor(1, 1, 1);
                if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor))
                    mat.baseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);

                // Metallic
                float metallic = 0.0f;
                if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metallic))
                    mat.metallic = metallic;

                // Roughness
                float roughness = 0.0f;
                if (AI_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
                    mat.roughness = roughness;

                // Emissive color/intensity
                aiColor3D emissiveColor(0, 0, 0);
                float emissiveIntensity = 1.0f;
                if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor))
                    mat.emission = glm::length(glm::vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b));
                if (AI_SUCCESS == material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity))
                    mat.emission *= emissiveIntensity;

                // Ambient occlusion (usually only as texture, not scalar)
                mat.ao = 1.0f;

                asset.materials.push_back(mat);
            }

            //Create output directory
            std::filesystem::create_directories(asset_info.shipped_path.parent_path());

            //Export model
            ExportModel(asset, asset_info.shipped_path);

            std::cout << "[Model] Compiled " << asset_info.raw_path.string() << " -> " << asset_info.shipped_path.string() << std::endl;

            //Update desc file with hashing
            desc_file.hash = fileHashing(asset_info.raw_path);
        }

        std::string Compiler::GetCuttlefishExecutable() const {

            //Locate all possible cuttlfish executables
            std::vector<std::filesystem::path> possible_paths = {
                exec_path / "cuttlefish/cuttlefish.exe"
            };

            for (const auto& path : possible_paths) {
                if (std::filesystem::exists(path)) {
                    std::cout << "Found cuttlefish: " << path << std::endl;
                    return path.string();
                }
            }
        
            std::cout << "WARNING: Cuttlefish executable not found!" << std::endl;
            return "cuttlefish.exe"; // Fallback
        }

        bool Compiler::CompressTextureDDS(unsigned char* pixels, int width, int height, int channels,
            const std::string& output_path, const std::string& format,
            const nlohmann::json& settings) const {
            try {
                std::string cuttlefish_exe = GetCuttlefishExecutable();

                // Create temporary PNG file (easier for cuttlefish to handle)
                std::string temp_input = "temp_" + std::to_string(getCurrentTimeStamp()) + ".png";
                if (!stbi_write_png(temp_input.c_str(), width, height, 4, pixels, width * 4)) {
                    std::cout << "Failed to write temporary PNG" << std::endl;
                    return false;
                }

                std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());

                // WINDOWS SYSTEM() REQUIRES SPECIAL QUOTING [web:1061]
                std::stringstream cmd;

                // Method: Wrap ENTIRE command in outer quotes for Windows system()
                cmd << "\"";  // Start outer quotes
                cmd << "\"" << cuttlefish_exe << "\"";  // Quoted executable
                cmd << " -i \"" << temp_input << "\"";
                cmd << " -f " << format;
                cmd << " -Q " << settings.value("quality", "normal");
                cmd << " -s rgba";
                cmd << " -o \"" << output_path << "\"";
                cmd << " --file-format dds";
                cmd << " --create-dir";

                if (settings.value("generate_mipmaps", true)) {
                    cmd << " -m";
                }

                cmd << "\"";  // End outer quotes

                std::string final_command = cmd.str();
                std::cout << "Running: " << final_command << std::endl;

                int result = system(final_command.c_str());

                // Clean up temp file
                std::filesystem::remove(temp_input);

                if (result == 0) {
                    std::cout << "DDS compression successful" << std::endl;
                }
                else {
                    std::cout << "DDS compression failed with code: " << result << std::endl;
                }

                return result == 0;
            }
            catch (const std::exception& e) {
                std::cout << "Cuttlefish DDS compression failed: " << e.what() << std::endl;
                return false;
            }
        }

        std::string Compiler::GetASTCEncoderExecutable() const {

            //Locate all possible astcenc executables
            std::vector<std::filesystem::path> possible_paths = {
                exec_path / "astc/astcenc-avx2.exe",
                exec_path / "astc/astcenc-neon.exe",
                exec_path / "astc/astcenc-native.exe",
                exec_path / "astc/astcenc-sse2.exe"
            };

            for (const auto& path : possible_paths) {
                if (std::filesystem::exists(path)) {
                    std::cout << "Found ASTC encoder: " << path << std::endl;
                    return path.string();
                }
            }

            std::cout << "WARNING: astcenc executable not found!" << std::endl;
            return "astcenc-avx2.exe"; // Fallback
        }

        std::string Compiler::ConvertToASTCBlockSize(const std::string& format) const {
            // Convert format like "ASTC_4x4" to "4x4" for astcenc
            if (format.find("ASTC_") == 0) {
                return format.substr(5); // Remove "ASTC_" prefix
            }

            // Default mappings
            if (format == "ASTC_4x4") return "4x4";
            if (format == "ASTC_6x6") return "6x6";
            if (format == "ASTC_8x8") return "8x8";

            return "4x4"; // Default fallback
        }

        bool Compiler::CompressTextureASTC(unsigned char* pixels, int width, int height, int channels,
            const std::string& output_path, const std::string& format,
            const nlohmann::json& settings) const {
            try {
                // Get astcenc executable (ARM's ASTC Encoder)
                std::string astcenc_exe = GetASTCEncoderExecutable();

                // Create temporary PNG input (astcenc works best with standard image formats)
                std::string temp_input = "temp_astc_" + std::to_string(getCurrentTimeStamp()) + ".png";
                if (!stbi_write_png(temp_input.c_str(), width, height, 4, pixels, width * 4)) {
                    std::cout << "Failed to write temporary PNG for ASTC" << std::endl;
                    return false;
                }

                std::filesystem::create_directories(std::filesystem::path(output_path).parent_path());

                // Build astcenc command with proper syntax
                std::stringstream cmd;

                // Windows system() double-quote wrapping
                cmd << "\"";  // Start outer quotes for Windows
                cmd << "\"" << astcenc_exe << "\"";  // Quoted executable path

                // astcenc syntax: astcenc -cl input.png output.astc block_size quality
                cmd << " -cl";  // Compress LDR (Low Dynamic Range)
                cmd << " \"" << temp_input << "\"";  // Input file
                cmd << " \"" + output_path + "\"";   // Output file
                cmd << " " << ConvertToASTCBlockSize(format);  // Block size (e.g., "4x4")
                cmd << " -" << settings.value("quality", "medium");  // Quality: -fastest, -fast, -medium, -thorough, -exhaustive
                cmd << "\"";  // End outer quotes for Windows

                std::string final_command = cmd.str();
                std::cout << "Running ASTC: " << final_command << std::endl;

                int result = system(final_command.c_str());

                // Clean up temp file
                std::filesystem::remove(temp_input);

                if (result == 0) {
                    std::cout << "ASTC compression successful" << std::endl;
                }
                else {
                    std::cout << "ASTC compression failed with code: " << result << std::endl;
                }

                return result == 0;
            }
            catch (const std::exception& e) {
                std::cout << "ASTC compression failed: " << e.what() << std::endl;
                return false;
            }
        }

        std::string Compiler::GetFFMPEGExecutable() const {
            //Locate all possible ffmpeg executables
            std::vector<std::filesystem::path> possible_paths = {
                exec_path / "ffmpeg/ffmpeg.exe"
            };

            for (const auto& path : possible_paths) {
                if (std::filesystem::exists(path)) {
                    std::cout << "Found ffmpeg: " << path << std::endl;
                    return path.string();
                }
            }

            std::cout << "WARNING: ffmpegffmpeg executable not found!" << std::endl;
            return "ffmpeg.exe"; // Fallback
        }

        void Compiler::ExportModel(const Model& asset, const std::filesystem::path& out_path) const {
            std::ofstream out(out_path, std::ios::binary);

            // Write bounding box
            out.write((char*)&asset.aabbMin, sizeof(asset.aabbMin));
            out.write((char*)&asset.aabbMax, sizeof(asset.aabbMax));

            // Write LODs
            uint32_t lodCount = (uint32_t)asset.lods.size();
            out.write((char*)&lodCount, sizeof(lodCount));
            out.write((char*)asset.lods.data(), lodCount * sizeof(uint32_t));

            // Write vertices/indices
            uint32_t vtxCount = (uint32_t)asset.vertices.size();
            uint32_t idxCount = (uint32_t)asset.indices.size();
            out.write((char*)&vtxCount, sizeof(vtxCount));
            out.write((char*)&idxCount, sizeof(idxCount));
            out.write((char*)asset.vertices.data(), vtxCount * sizeof(Vertex));
            out.write((char*)asset.indices.data(), idxCount * sizeof(uint32_t));

            // Write submeshes
            uint32_t submeshCount = (uint32_t)asset.submeshes.size();
            out.write((char*)&submeshCount, sizeof(submeshCount));
            for (const Submesh& sm : asset.submeshes) {
                uint32_t nameLen = (uint32_t)sm.name.size();
                out.write((char*)&nameLen, sizeof(nameLen));
                out.write(sm.name.data(), nameLen);
                out.write((char*)&sm.materialIndex, sizeof(sm.materialIndex));
                out.write((char*)&sm.firstIndex, sizeof(sm.firstIndex));
                out.write((char*)&sm.indexCount, sizeof(sm.indexCount));
                out.write((char*)&sm.vertexOffset, sizeof(sm.vertexOffset));
            }

            // Write morph targets
            uint32_t morphCount = (uint32_t)asset.morphTargets.size();
            out.write((char*)&morphCount, sizeof(morphCount));
            for (const MorphTarget& mt : asset.morphTargets) {
                uint32_t nameLen = (uint32_t)mt.name.size();
                out.write((char*)&nameLen, sizeof(nameLen));
                out.write(mt.name.data(), nameLen);

                uint32_t deltaCount = (uint32_t)mt.positionDeltas.size();
                out.write((char*)&deltaCount, sizeof(deltaCount));
                out.write((char*)mt.positionDeltas.data(), deltaCount * sizeof(glm::vec3));
                out.write((char*)mt.normalDeltas.data(), deltaCount * sizeof(glm::vec3));
            }

            // Write skeleton bones
            uint32_t boneCount = (uint32_t)asset.skeleton.size();
            out.write((char*)&boneCount, sizeof(boneCount));
            for (const Bone& b : asset.skeleton) {
                uint32_t nameLen = (uint32_t)b.name.size();
                out.write((char*)&nameLen, sizeof(nameLen));
                out.write(b.name.data(), nameLen);
                out.write((char*)&b.parent, sizeof(b.parent));
                out.write((char*)&b.bindPose, sizeof(glm::mat4));
            }

            // Write animations
            uint32_t animCount = (uint32_t)asset.animations.size();
            out.write((char*)&animCount, sizeof(animCount));
            for (const AnimationClip& anim : asset.animations) {
                uint32_t nameLen = (uint32_t)anim.name.size();
                out.write((char*)&nameLen, sizeof(nameLen));
                out.write(anim.name.data(), nameLen);
                out.write((char*)&anim.duration, sizeof(anim.duration));
                out.write((char*)&anim.isAdditive, sizeof(anim.isAdditive));

                uint32_t trackCount = (uint32_t)anim.tracks.size();
                out.write((char*)&trackCount, sizeof(trackCount));
                for (const AnimationTrack& track : anim.tracks) {
                    uint32_t boneLen = (uint32_t)track.boneName.size();
                    out.write((char*)&boneLen, sizeof(boneLen));
                    out.write(track.boneName.data(), boneLen);

                    uint32_t keyCount = (uint32_t)track.keys.size();
                    out.write((char*)&keyCount, sizeof(keyCount));
                    for (const AnimationKey& key : track.keys) {
                        out.write((char*)&key.time, sizeof(key.time));
                        out.write((char*)&key.translation, sizeof(key.translation));
                        out.write((char*)&key.rotation, sizeof(key.rotation));
                        out.write((char*)&key.scale, sizeof(key.scale));
                        // Write morph target weights if blend shapes exist
                        uint32_t morphWeightsCount = (uint32_t)key.morphTargetWeights.size();
                        out.write((char*)&morphWeightsCount, sizeof(morphWeightsCount));
                        out.write((char*)key.morphTargetWeights.data(), morphWeightsCount * sizeof(float));
                    }
                }
            }

            // Write materials
            uint32_t matCount = (uint32_t)asset.materials.size();
            out.write((char*)&matCount, sizeof(matCount));
            for (const Material& mat : asset.materials) {
                uint32_t nameLen = (uint32_t)mat.name.size();
                out.write((char*)&nameLen, sizeof(nameLen));
                out.write(mat.name.data(), nameLen);

                auto writeStr = [&](const std::string& str) {
                    uint32_t len = (uint32_t)str.size();
                    out.write((char*)&len, sizeof(len));
                    out.write(str.data(), len);
                    };

                writeStr(mat.diffuseMap);
                writeStr(mat.normalMap);
                writeStr(mat.metallicMap);
                writeStr(mat.roughnessMap);
                writeStr(mat.aoMap);
                writeStr(mat.emissionMap);

                out.write((char*)&mat.baseColor, sizeof(mat.baseColor));
                out.write((char*)&mat.metallic, sizeof(mat.metallic));
                out.write((char*)&mat.roughness, sizeof(mat.roughness));
                out.write((char*)&mat.ao, sizeof(mat.ao));
                out.write((char*)&mat.emission, sizeof(mat.emission));
            }

            out.close();
        }

        bool Compiler::needsRecompilation(Info const& asset_info, Descriptor const& desc_file) const {

            //Get shipped path
            auto shipped = asset_info.shipped_path;

            //Check shipped asset exists
            if (!std::filesystem::exists(shipped))
                return true;

            //Compare source and output timestamps
            auto raw_time = std::filesystem::last_write_time(asset_info.raw_path);
            auto shipped_time = std::filesystem::last_write_time(shipped);
            if (raw_time > shipped_time)
                return true;

            //Compare content hashes for safety
            if (fileHashing(asset_info.raw_path) != desc_file.hash)
                return true;

            //All up to date
            return false;
        }

		void Compiler::processAsset(Info& asset_info) {

            //Check for desc files and output
            auto asset_desc_path = assets_root / asset_info.relative_folder / (asset_info.raw_path.filename().string() + desc_ext);

            //Get desc obj
            Descriptor desc_obj;

            //Check if desc exists
            if (!std::filesystem::exists(asset_desc_path)) {
                desc_obj = createDefaultDesc(asset_info, asset_desc_path);
            }
            else {
                desc_obj = readDescFile(asset_info, asset_desc_path);
            }

            //Check if asset is compilable
            if (Assets::isAssetCompilable(asset_info.type)) {

                //Compiling operation
                compileAndShip(desc_obj, asset_info);
            }
            else {

                //If asset is not compilable ship asset straight into 
                asset_info.shipped_path = output_dir / asset_info.relative_folder / asset_info.name;

                //Only ship if source if updates are needed
                if (needsRecompilation(asset_info, desc_obj)) {

                    //Copy all assets
                    copyFile(asset_info.raw_path, asset_info.shipped_path);

                    //Update hashing
                    desc_obj.hash = fileHashing(asset_info.raw_path);
                }
            }

            //Double check if there are desc changes
            if (desc_obj != readDescFile(asset_info, asset_desc_path)) {
                saveDescFile(desc_obj, asset_desc_path);
            }
		}
	}
}

#endif
