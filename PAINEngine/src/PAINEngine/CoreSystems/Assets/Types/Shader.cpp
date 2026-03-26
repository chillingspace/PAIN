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
        void Shader::SetUniform(const std::string& name, const glm::mat4& m)
        {
            //Location variable
            GLint location = -1;

            //Check loc map
            if (loc_map.count(name)) {
                location = loc_map[name];
            }
            else {
                location = glGetUniformLocation(m_RendererID, name.c_str());
                if (location == -1) {
                    PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                    return;
                }
                else {
                    loc_map[name] = location;
                }
            }
            glUniformMatrix4fv(location, 1, GL_FALSE, &m[0][0]);
        }

        void Shader::SetUniform(const std::string& name, const glm::mat3& m)
        {
            GLint location = -1;

            if (loc_map.count(name)) {
                location = loc_map[name];
            }
            else {
                location = glGetUniformLocation(m_RendererID, name.c_str());
                if (location == -1) {
                    PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                    return;
                }
                else {
                    loc_map[name] = location;
                }
            }
            glUniformMatrix3fv(location, 1, GL_FALSE, &m[0][0]);
        }

        void Shader::SetUniform(const std::string& name, const glm::vec4& val)
        {
            //Location variable
            GLint location = -1;

            //Check loc map
            if (loc_map.count(name)) {
                location = loc_map[name];
            }
            else {
                location = glGetUniformLocation(m_RendererID, name.c_str());
                if (location == -1) {
                    PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                    return;
                }
                else {
                    loc_map[name] = location;
                }
            }
            glUniform4f(location, val.x, val.y, val.z, val.w);
        }

        void Shader::SetUniform(const std::string& name, const glm::vec3& val)
        {
            //Location variable
            GLint location = -1;

            //Check loc map
            if (loc_map.count(name)) {
                location = loc_map[name];
            }
            else {
                location = glGetUniformLocation(m_RendererID, name.c_str());
                if (location == -1) {
                    PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                    return;
                }
                else {
                    loc_map[name] = location;
                }
            }
            glUniform3f(location, val.x, val.y, val.z);
        }

        void Shader::SetUniform(const std::string& name, const glm::vec2& val)
        {
            //Location variable
            GLint location = -1;

            //Check loc map
            if (loc_map.count(name)) {
                location = loc_map[name];
            }
            else {
                location = glGetUniformLocation(m_RendererID, name.c_str());
                if (location == -1) {
                    PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                    return;
                }
                else {
                    loc_map[name] = location;
                }
            }
            glUniform2f(location, val.x, val.y);
        }

        void Shader::SetUniform(const std::string& name, float x, float y, float z)
        {
            //Location variable
            GLint location = -1;

            //Check loc map
            if (loc_map.count(name)) {
                location = loc_map[name];
            }
            else {
                location = glGetUniformLocation(m_RendererID, name.c_str());
                if (location == -1) {
                    PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                    return;
                }
                else {
                    loc_map[name] = location;
                }
            }
            glUniform3f(location, x, y, z);
        }

        void Shader::SetUniform(const std::string& name, float val)
        {
            //Location variable
            GLint location = -1;

            //Check loc map
            if (loc_map.count(name)) {
                location = loc_map[name];
            }
            else {
                location = glGetUniformLocation(m_RendererID, name.c_str());
                if (location == -1) {
                    PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                    return;
                }
                else {
                    loc_map[name] = location;
                }
            }
            glUniform1f(location, val);
        }

        void Shader::SetUniform(const std::string& name, int val)
        {
            //Location variable
            GLint location = -1;

            //Check loc map
            if (loc_map.count(name)) {
                location = loc_map[name];
            }
            else {
                location = glGetUniformLocation(m_RendererID, name.c_str());
                if (location == -1) {
                    PN_CORE_WARN("Uniform '{}' does not exist in shader program {}", name, m_RendererID);
                    return;
                }
                else {
                    loc_map[name] = location;
                }
            }
            glUniform1i(location, val);
        }

    }
}