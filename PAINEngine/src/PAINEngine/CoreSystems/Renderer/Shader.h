#pragma once

namespace PAIN {
	class Shader {

	public:
		Shader(const std::string& vertex_src, const std::string& fragment_src);
		~Shader();

		void Bind() const;
		void UnBind() const;

		unsigned int GetRendererID() const { return m_RendererID; }
	private:
		uint32_t m_RendererID = 0;

		uint32_t CompileShader(unsigned int type, const std::string& source);
		uint32_t LinkProgram(unsigned int vert_shader, unsigned int frag_shader);
	};
}