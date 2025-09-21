#include "pch.h"
#include "Shader.h"

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

	static bool CheckShader(GLuint shader, const char* label) {
		GLint compiled = GL_FALSE;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled == GL_TRUE) return true;

		GLint len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		std::string log(len ? len - 1 : 0, '\0');
		if (len > 1) glGetShaderInfoLog(shader, len, nullptr, log.data());
		PN_CORE_ERROR("[Shader] Compile failed ({0}):\n{1}", label, log);
		return false;
	}

	static bool CheckProgram(GLuint program) {
		GLint linked = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (linked == GL_TRUE) return true;

		GLint len = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		std::string log(len ? len - 1 : 0, '\0');
		if (len > 1) glGetProgramInfoLog(program, len, nullptr, log.data());
		PN_CORE_ERROR("[Shader] Link failed:\n{0}", log);
		return false;
	}

	uint32_t Shader::LinkProgram(unsigned int vert_shader, unsigned int frag_shader)
	{
		GLuint program = glCreateProgram();
		glAttachShader(program, vert_shader);
		glAttachShader(program, frag_shader);
		glLinkProgram(program);

		if (!CheckProgram(program)) {
			PN_CORE_ERROR("FAILED");
			assert("Program link failed");
		}

		// (Optional but recommended)
		glDetachShader(program, vert_shader);
		glDetachShader(program, frag_shader);

		return program;
	}
}