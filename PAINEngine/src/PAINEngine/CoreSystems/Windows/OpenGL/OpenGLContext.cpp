#include "pch.h"

#ifdef PN_PLATFORM_WINDOWS
#include "OpenGLContext.h"

namespace PAIN {
	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}
	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);

		if (glfwGetCurrentContext() != m_WindowHandle) {
			PN_CORE_ERROR("GLFW context is NOT current for window handle! Possible error during make current.");
			throw std::runtime_error("GLFW context is not current.");
		}

		//Creating window
		GLenum err = glewInit();

		if (err != GLEW_OK) {
			PN_CORE_ERROR("GLEW init failed: {}", (const char*)glewGetErrorString(err));
			throw std::runtime_error("GLEW init failed");
		}

		// Check if we actually got a debug context
		GLint flags = 0;
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
			// Enable debug output
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(
				[](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
					const GLchar* message, const void* userParam)
				{
					PN_CORE_ERROR("[GL DEBUG] src=0x{0:x} type=0x{1:x} id={2} sev=0x{3:x}: {4}",
						source, type, id, severity, message);
				}
			, nullptr);
			// Allow all messages (filter later if noisy)
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);


		}

		PN_CORE_INFO("OpenGL Info:");
		PN_CORE_INFO(" Vendor: {}", (const char*)glGetString(GL_VENDOR));
		PN_CORE_INFO(" Renderer: {}", (const char*)glGetString(GL_RENDERER));
		PN_CORE_INFO(" Version: {}", (const char*)glGetString(GL_VERSION));

	}
	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}

#endif