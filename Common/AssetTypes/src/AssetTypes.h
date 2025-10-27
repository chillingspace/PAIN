#pragma once

#ifndef ASSET_TYPES_HPP
#define ASSET_TYPES_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace PAIN {
    namespace Assets {

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