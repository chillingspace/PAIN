#pragma once

#ifdef PN_PLATFORM_WINDOWS

#include "pch.h"
#include "../Shader.h"
#include "../../../Applications/AppSystem.h"

namespace PAIN {
	class WindowsRenderer {
	public:
		WindowsRenderer();
		~WindowsRenderer();

		void Init();
		void Render();
		void Cleanup();

		/*bool InitGLFW();
		void SetWindow(GLFWwindow* window);
		GLFWwindow* GetWindow() const { return window; };*/

	private:

		struct Vertex {
			glm::vec3 pos;
			glm::vec3 color;
			glm::vec3 normal;
		};

		bool createBuffers();

		float clearColor[3];

		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ebo = 0;

		std::unique_ptr<Shader> m_shader = nullptr;
	};
}

#endif // PN_PLATFORM_WINDFOWS