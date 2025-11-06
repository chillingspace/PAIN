#include "pch.h"
#include "Shader.h"

namespace PAIN {
    namespace Assets {

        Shader::~Shader()
        {
            glDeleteProgram(m_RendererID);
        }

        void Shader::Bind() const
        {
            glUseProgram(m_RendererID);
        }

        void Shader::UnBind() const
        {
            glUseProgram(0);
        }

        // SET UNIFORMS HELPERS
        void Shader::SetUniform(const std::string& name, const glm::mat4& m) const
        {
            GLint location = glGetUniformLocation(m_RendererID, name.c_str());
            if (location == -1) {
                PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                return;
            }
            glUniformMatrix4fv(location, 1, GL_FALSE, &m[0][0]);
        }

        void Shader::SetUniform(const std::string& name, const glm::vec4& val) const
        {
            GLint location = glGetUniformLocation(m_RendererID, name.c_str());
            if (location == -1) {
                PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                return;
            }
            glUniform4f(location, val.x, val.y, val.z, val.w);
        }

        void Shader::SetUniform(const std::string& name, const glm::vec3& val) const
        {
            GLint location = glGetUniformLocation(m_RendererID, name.c_str());
            if (location == -1) {
                PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                return;
            }
            glUniform3f(location, val.x, val.y, val.z);
        }

        void Shader::SetUniform(const std::string& name, const glm::vec2& val) const
        {
            GLint location = glGetUniformLocation(m_RendererID, name.c_str());
            if (location == -1) {
                PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                return;
            }
            glUniform2f(location, val.x, val.y);
        }

        void Shader::SetUniform(const std::string& name, float x, float y, float z) const
        {
            GLint location = glGetUniformLocation(m_RendererID, name.c_str());
            if (location == -1) {
                PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                return;
            }
            glUniform3f(location, x, y, z);
        }

        void Shader::SetUniform(const std::string& name, float val) const
        {
            GLint location = glGetUniformLocation(m_RendererID, name.c_str());
            if (location == -1) {
                PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                return;
            }
            glUniform1f(location, val);
        }

        void Shader::SetUniform(const std::string& name, int val) const
        {
            GLint location = glGetUniformLocation(m_RendererID, name.c_str());
            if (location == -1) {
                PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                return;
            }
            glUniform1i(location, val);
        }

    }
}