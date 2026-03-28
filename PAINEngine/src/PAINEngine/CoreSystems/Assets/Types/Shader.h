#pragma once

#ifndef ASSETS_SHADER_HPP
#define ASSETS_SHADER_HPP

#include "AssetTypes.h"

namespace PAIN {
	namespace Assets {

        //Shader class
        struct Shader : public IAsset {
        private:
            uint32_t m_RendererID = 0;

            //Shader loc cached
            std::unordered_map<std::string, GLint> loc_map;
        public:

            Shader() = default;
            ~Shader();
            Shader(uint32_t const& m_RendererID) : m_RendererID{ m_RendererID } {}
            Shader(const Shader&) = delete;
            Shader& operator=(const Shader&) = delete;

            Shader(Shader&& other) noexcept : m_RendererID(other.m_RendererID), loc_map(std::move(other.loc_map)) {
                other.m_RendererID = 0;
            }

            unsigned int GetRendererID() const { return m_RendererID; }

            void Bind() const;
            void UnBind() const;

            // Set uniform helpers

            // Set a uniform var of type glm::mat4
            void SetUniform(const std::string& name, const glm::mat4& m);

            // Set a uniform var of type glm::mat3
            void SetUniform(const std::string& name, const glm::mat3& m);

            // Set a uniform var of type glm::vec4
            void SetUniform(const std::string& name, const glm::vec4& val);

            // Set a uniform var of type glm::vec3
            void SetUniform(const std::string& name, const glm::vec3& val);

            void SetUniform(const std::string& name, const glm::vec2& val);

            // Set a uniform var of type glm::vec3 (float x , y , z)
            void SetUniform(const std::string& name, float x, float y, float z);

            // Set a uniform var of type float
            void SetUniform(const std::string& name, float val);

            // Set a uniform var of type int
            void SetUniform(const std::string& name, int val);
        };
	}
}

#endif
