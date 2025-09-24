#pragma once

#ifdef PN_PLATFORM_WINDOWS

#include "CoreSystems/Windows/GraphicsContext.h"

struct GLFWwindow;

namespace PAIN {

	class OpenGLContext : public GraphicsContext
	{
		public:
			OpenGLContext(GLFWwindow* windowHandle);

			virtual void Init() override;
			virtual void SwapBuffers() override;
		private:
			GLFWwindow* m_WindowHandle;
	};

}

#endif