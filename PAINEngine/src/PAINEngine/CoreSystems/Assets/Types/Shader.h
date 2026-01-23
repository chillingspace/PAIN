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
        public:

            Shader() = default;
            ~Shader();
            Shader(uint32_t const& m_RendererID) : m_RendererID{ m_RendererID } {}
            Shader(const Shader&) = delete;
            Shader& operator=(const Shader&) = delete;

            Shader(Shader&& other) noexcept : m_RendererID(other.m_RendererID) {
                other.m_RendererID = 0;
            }

            unsigned int GetRendererID() const { return m_RendererID; }

            void Bind() const;
            void UnBind() const;

            // Set uniform helpers

            // Set a uniform var of type glm::mat4
            void SetUniform(const std::string& name, const glm::mat4& m) const;

            // Set a uniform var of type glm::vec4
            void SetUniform(const std::string& name, const glm::vec4& val) const;

            // Set a uniform var of type glm::vec3
            void SetUniform(const std::string& name, const glm::vec3& val) const;

            void SetUniform(const std::string& name, const glm::vec2& val) const;

            // Set a uniform var of type glm::vec3 (float x , y , z)
            void SetUniform(const std::string& name, float x, float y, float z) const;

            // Set a uniform var of type float
            void SetUniform(const std::string& name, float val) const;

            // Set a uniform var of type int
            void SetUniform(const std::string& name, int val) const;
        };
	}
}

#endif
