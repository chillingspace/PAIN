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
			glUniformMatrix4fv(glGetUniformLocation(m_RendererID, name.c_str()), 1, GL_FALSE, &m[0][0]);
		}

		void Shader::SetUniform(const std::string& name, const glm::vec4& val) const
		{
			glUniform4f(glGetUniformLocation(m_RendererID, name.c_str()), val.x, val.y, val.z, val.w);
		}

		void Shader::SetUniform(const std::string& name, const glm::vec3& val) const
		{
			glUniform3f(glGetUniformLocation(m_RendererID, name.c_str()), val.x, val.y, val.z);
		}

		void Shader::SetUniform(const std::string& name, const glm::vec2& val) const
		{
			glUniform2f(glGetUniformLocation(m_RendererID, name.c_str()), val.x, val.y);
		}

		void Shader::SetUniform(const std::string& name, float x, float y, float z) const
		{
			glUniform3f(glGetUniformLocation(m_RendererID, name.c_str()), x, y, z);
		}

		void Shader::SetUniform(const std::string& name, float val) const
		{
			glUniform1f(glGetUniformLocation(m_RendererID, name.c_str()), val);
		}

		void Shader::SetUniform(const std::string& name, int val) const
		{
			glUniform1i(glGetUniformLocation(m_RendererID, name.c_str()), val);
		}
	}
}
