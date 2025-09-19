#include "CoreSystems/Windows/OpenGL/OpenGLContext.h"

namespace PAIN {
	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
	}
	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);


		//Creating window
		GLenum err = glewInit();

		if (err != GLEW_OK) {
			PN_CORE_ERROR("GLEW init failed: {}", (const char*)glewGetErrorString(err));
			throw std::runtime_error("GLEW init failed");
		}

		err = glGetError();
		if (err != GL_NO_ERROR) {
		}

		PN_CORE_INFO("OpenGL Info:");
		PN_CORE_INFO(" Vendor: {}", (const char*)glGetString(GL_VENDOR));
		PN_CORE_INFO(" Renderer: {}", (const char*)glGetString(GL_RENDERER));
		PN_CORE_INFO(" Version: {}", (const char*)glGetString(GL_VERSION));

	}
	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}
}