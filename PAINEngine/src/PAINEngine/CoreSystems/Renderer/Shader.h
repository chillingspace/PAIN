#pragma once

namespace PAIN {
	class Shader {

	public:
		Shader(const std::string& vertex_src, const std::string& fragment_src);
		~Shader();

		void Bind() const;
		void UnBind() const;

		// Set uniform helpers
		
		// Set a uniform var of type glm::mat4
		void SetUniform(const std::string& name, const glm::mat4& m) const;

		// Set a uniform var of type glm::vec4
		void SetUniform(const std::string& name, const glm::vec4& val) const;

		// Set a uniform var of type glm::vec3
		void SetUniform(const std::string& name, const glm::vec3& val) const;

		// Set a uniform var of type glm::vec3 (float x , y , z)
		void SetUniform(const std::string& name, float x, float y, float z) const;

		// Set a uniform var of type float
		void SetUniform(const std::string& name, float val) const;

		// Set a uniform var of type int
		void SetUniform(const std::string& name, int val) const;

		unsigned int GetRendererID() const { return m_RendererID; }
	private:
		uint32_t m_RendererID = 0;
		uint32_t CompileShader(unsigned int type, const std::string& source);
		uint32_t LinkProgram(unsigned int vert_shader, unsigned int frag_shader);
	};
}