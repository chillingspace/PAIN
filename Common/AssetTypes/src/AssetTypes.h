#pragma once

#ifndef ASSET_TYPES_HPP
#define ASSET_TYPES_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <memory>

#ifdef PN_PLATFORM_ANDROID
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>

#ifndef GL_COMPRESSED_RGBA_ASTC_4x4_KHR
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR       0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR       0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR       0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR       0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR       0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR       0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR       0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR       0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR      0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR      0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR      0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR     0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR     0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR     0x93BD
#endif

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#else
#include "GL/glew.h"
#endif

#include "AssetData.h"

namespace PAIN {
    namespace Assets {

        //Asset interface
        struct IAsset {
        public: 
            virtual ~IAsset() = default;

            //Details
            GUID guid;
            Type type;
            std::string name;

            //Asset relative folder
            std::filesystem::path main_relative_path;
            std::filesystem::path shipped_relative_path;
        };

        // Vertex structure suitable for PBR, skinning, and morph targets
        struct Vertex {
            glm::vec3 pos;
            glm::vec3 normal;
            glm::vec2 uv;
            glm::vec3 tangent;    // For normal mapping/PBR
            glm::vec3 bitangent;  // For normal mapping/PBR
            uint8_t boneIndices[4]; // Supports 4 bone influences per vertex
            float boneWeights[4];   // Matches bone indices, sum to 1
            glm::vec3 color;        // (optional) for vertex color
        };

        // Morph Target Data (for blend shapes)
        struct MorphTarget {
            std::string name;
            std::vector<glm::vec3> positionDeltas;
            std::vector<glm::vec3> normalDeltas;
            // Optionally: tangent, bitangent deltas
        };

        // Bone/Animation Structures
        struct Bone {
            std::string name;
            int parent;
            glm::mat4 bindPose;
        };

        struct AnimationKey {
            float time;
            glm::vec3 translation;
            glm::quat rotation;
            glm::vec3 scale;
            std::vector<float> morphTargetWeights; // Support for blend shapes
        };

        struct AnimationTrack {
            std::string boneName;
            std::vector<AnimationKey> keys;
        };

        struct AnimationClip {
            std::string name;
            float duration;
            std::vector<AnimationTrack> tracks;
            bool isAdditive; // For blending, layering
        };

        // Material (PBR support)
        struct Material {
            std::string name{};
            std::string diffuse_map_buf{};
            std::string normalMap{};
            std::string metallicMap{};
            std::string roughnessMap{};
            std::string aoMap{};
            std::string emissionMap{};
            std::string bumpMap{};
            std::string heightMap{};
            glm::vec3 baseColor{1.f, 0.f, 1.f};
            float metallic{0.1f};
            float roughness{0.1f};
            float emission{};
            // Additional: transparency, alpha mode, etc.

            // opengl textures
            unsigned int gl_diffuse_tex{};
            unsigned int gl_ao_tex{};
            unsigned int gl_emissive_tex{};
        };

        // Submesh: supports multi-material, LODs
        struct Submesh {
            std::string name;
            uint32_t materialIndex; // Refers to materials in Model
            uint32_t firstIndex;
            uint32_t indexCount;
            uint32_t vertexOffset;
            // Optionally: bounding box, LOD info
        };

        // Model class - full AAA-ready object
        struct Model : public IAsset {
            std::string vpath{};

            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Submesh> submeshes;
            std::vector<Bone> skeleton;
            std::vector<MorphTarget> morphTargets;
            std::vector<AnimationClip> animations;
            std::vector<Material> materials;

            // Extra: bounding box, LODs, instancing support, metadata, engine tags, etc.
            glm::vec3 aabbMin, aabbMax;
            std::vector<uint32_t> lods;
        };
    }
}

#endif