#include "CoreSystems/Renderer/Shader.h"

namespace PAIN {

	Shader::Shader(const std::string& vertex_src, const std::string& fragment_src)
	{
		uint32_t vert_shader = CompileShader(GL_VERTEX_SHADER, vertex_src);
		uint32_t frag_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_src);

		m_RendererID = LinkProgram(vert_shader, frag_shader);

		// Clean up shaders (they're linked now)
		glDeleteShader(vert_shader);
		glDeleteShader(frag_shader);
	}

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

	uint32_t Shader::CompileShader(unsigned int type, const std::string& source)
	{
		// Create vert & frag shaders
		uint32_t shader = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		// Check compilation
		int success;

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
			PN_CORE_ERROR("Shader Compilation Failed ({0}): {1}", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog);
		}

		return shader;

	}
	uint32_t Shader::LinkProgram(unsigned int vert_shader, unsigned int frag_shader)
	{
		// Create program
		uint32_t program = glCreateProgram();
		glAttachShader(program, vert_shader);
		glAttachShader(program, frag_shader);
		glLinkProgram(program);

		int success;
		glGetProgramiv(m_RendererID, GL_LINK_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetProgramInfoLog(m_RendererID, 512, nullptr, infoLog);
			PN_CORE_ERROR("Shader Program Linking Failed:\n{0}", infoLog);
		}

		return program;
	}
}