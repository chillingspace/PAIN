#include "pch.h"
#include "Shader.h"

namespace PAIN {
	Shader::Shader() : m_RendererID(0)
	{
	}

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
		PN_CORE_INFO("Shader destructor called for program ID: {}", m_RendererID);
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

	uint32_t Shader::CompileShader(unsigned int type, const std::string& source)
	{
		// Create vert & frag shaders
		uint32_t shader = glCreateShader(type);
		const char* src = source.c_str();

		PN_CORE_INFO("Compiling {} shader", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
		// PN_CORE_INFO("Shader source:\n{0}", source);

		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		// Check compilation
		int success;

		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			char infoLog[512];
			glGetShaderInfoLog(shader, 512, nullptr, infoLog);
#ifdef PN_PLATFORM_ANDROID
			PN_CORE_ERROR("Shader compile error {0}: {1}",
				type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog);
#else
			PN_CORE_ERROR("Shader Compilation Failed ({0}): {1}", type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT", infoLog);
#endif

			assert(false);
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
#ifdef PN_PLATFORM_ANDROID
		PN_CORE_ERROR("[Shader] Compile failed ({0}):\n{1}", label, log);
#else
		PN_CORE_ERROR("[Shader] Compile failed ({0}):\n{1}", label, log);
#endif
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
#ifdef PN_PLATFORM_ANDROID
		PN_CORE_ERROR("[Shader] Link failed:\n{0}", log);
#else
		PN_CORE_ERROR("[Shader] Link failed:\n{0}", log);
#endif
		return false;
	}

	uint32_t Shader::LinkProgram(unsigned int vert_shader, unsigned int frag_shader)
	{
		GLuint program = glCreateProgram();
		glAttachShader(program, vert_shader);
		glAttachShader(program, frag_shader);
		glLinkProgram(program);

		GLint numUniforms = 0;
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
		PN_CORE_INFO("Linked program {} has {} active uniforms", program, numUniforms);

		for (int i = 0; i < numUniforms; i++) {
			char name[256] = { 0 };
			GLsizei length = 0;
			GLint size = 0;
			GLenum type = 0;
			glGetActiveUniform(program, i, 256, &length, &size, &type, name);
			PN_CORE_INFO("  Uniform {}: '{}'", i, name);
		}

		if (!CheckProgram(program)) {
#ifdef PN_PLATFORM_ANDROID
			PN_CORE_ERROR("Program link FAILED");
#else
			PN_CORE_ERROR("Program link FAILED");
#endif
			assert("Program link failed");
		}
		else {
			PN_CORE_INFO("Program link succeeded");
		}

		// Test getting uniform location RIGHT NOW
		GLint testLoc = glGetUniformLocation(program, "projection");
		PN_CORE_INFO("Projection location immediately after link: {}", testLoc);

		// (Optional but recommended)
		glDetachShader(program, vert_shader);
		glDetachShader(program, frag_shader);

		return program;
	}
}