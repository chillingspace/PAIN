#pragma once

#ifndef ASSET_TYPES_HPP
#define ASSET_TYPES_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <memory>

#include "GL/glew.h"

namespace PAIN {
    namespace Assets {

        //Texture format
        enum class TextureFormat {
            UNKNOWN, BC7, ASTC_4x4, // add more as needed
        };

        //Texture class
        class Texture {
        public:
            int width = 0, height = 0, mips = 1;
            TextureFormat format = TextureFormat::BC7;
            unsigned int glTexFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            std::vector<uint8_t> data;
            GLuint gl_texture = 0;

            ~Texture() { if (gl_texture) glDeleteTextures(1, &gl_texture); }
        };

        //Model data
        struct Vertex { glm::vec3 pos, normal; glm::vec2 uv; };
        struct BoneWeight { uint32_t boneIndex; float weight; };
        struct Bone { std::string name; int parent; glm::mat4 bindPose; };
        struct AnimationKey { float time; glm::vec3 translation; glm::quat rotation; glm::vec3 scale; };
        struct AnimationTrack { std::string boneName; std::vector<AnimationKey> keys; };
        struct AnimationClip { std::string name; float duration; std::vector<AnimationTrack> tracks; };
        struct Material { std::string name; std::string diffuseMap; std::string normalMap; /* etc. */ };

        //Model class
        struct Model {
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Bone> skeleton;
            std::vector<std::vector<BoneWeight>> weights;
            std::vector<AnimationClip> animations;
            std::vector<Material> materials;
        };

    }
}

#endif