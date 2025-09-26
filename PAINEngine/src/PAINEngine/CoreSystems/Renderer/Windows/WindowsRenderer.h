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

		static constexpr Vertex vertices[] = {
			// Front (+Z)
			{{-0.5f, -0.5f,  0.5f}, {1,0,0}, {0,0,1}},
			{{ 0.5f, -0.5f,  0.5f}, {0,1,0}, {0,0,1}},
			{{ 0.5f,  0.5f,  0.5f}, {0,0,1}, {0,0,1}},
			{{-0.5f,  0.5f,  0.5f}, {1,1,0}, {0,0,1}},

			// Back (-Z)
			{{ 0.5f, -0.5f, -0.5f}, {1,0,1}, {0,0,-1}},
			{{-0.5f, -0.5f, -0.5f}, {0,1,1}, {0,0,-1}},
			{{-0.5f,  0.5f, -0.5f}, {1,1,1}, {0,0,-1}},
			{{ 0.5f,  0.5f, -0.5f}, {0,0,0}, {0,0,-1}},

			// Left (-X)
			{{-0.5f, -0.5f, -0.5f}, {1,0,0}, {-1,0,0}},
			{{-0.5f, -0.5f,  0.5f}, {0,1,0}, {-1,0,0}},
			{{-0.5f,  0.5f,  0.5f}, {0,0,1}, {-1,0,0}},
			{{-0.5f,  0.5f, -0.5f}, {1,1,0}, {-1,0,0}},

			// Right (+X)
			{{ 0.5f, -0.5f,  0.5f}, {1,0,1}, {1,0,0}},
			{{ 0.5f, -0.5f, -0.5f}, {0,1,1}, {1,0,0}},
			{{ 0.5f,  0.5f, -0.5f}, {1,1,1}, {1,0,0}},
			{{ 0.5f,  0.5f,  0.5f}, {0,0,0}, {1,0,0}},

			// Top (+Y)
			{{-0.5f,  0.5f,  0.5f}, {1,0,0}, {0,1,0}},
			{{ 0.5f,  0.5f,  0.5f}, {0,1,0}, {0,1,0}},
			{{ 0.5f,  0.5f, -0.5f}, {0,0,1}, {0,1,0}},
			{{-0.5f,  0.5f, -0.5f}, {1,1,0}, {0,1,0}},

			// Bottom (-Y)
			{{-0.5f, -0.5f, -0.5f}, {1,0,1}, {0,-1,0}},
			{{ 0.5f, -0.5f, -0.5f}, {0,1,1}, {0,-1,0}},
			{{ 0.5f, -0.5f,  0.5f}, {1,1,1}, {0,-1,0}},
			{{-0.5f, -0.5f,  0.5f}, {0,0,0}, {0,-1,0}}
		};

		static constexpr unsigned int indices[] = {
			// Front (+Z)
			0,1,2, 0,2,3,
			// Back (-Z)
			4,5,6, 4,6,7,
			// Left (-X)
			8,9,10, 8,10,11,
			// Right (+X)
			12,13,14, 12,14,15,
			// Top (+Y)
			16,17,18, 16,18,19,
			// Bottom (-Y)
			20,21,22, 20,22,23
		};

		struct Material {
			float rough;
			float metal;
			glm::vec3 color;
		};

		struct Light {
			glm::vec3 position;
			float L_intensity;
		};

		static constexpr Material material = {
			0.f,		// 0 -> smooth, 1 -> rough
			1.f,
			{1.f, 0.71f, 0.29f}	// gold-like
		};

		static constexpr Light light = {
			{0.f, 0.f, 0.f},	// position
			1.f					// intensity
		};

		static constexpr float ao = 1.f;		// ambient occlusion	(1 = no occlusion)

		bool createBuffers();

		float clearColor[3];

		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ebo = 0;

		std::unique_ptr<Shader> m_shader = nullptr;
	};

	class Camera {
	public:
		glm::vec3 pos{ 0.f, 0.f, 10.f };
		glm::vec3 target{ 0.f, 0.f, 0.f };
		glm::vec3 up{ 0.f, 1.f, 0.f };

		float fov{ 90.f };
		float near_plane{ 0.1f };		// closest distance camera can see
		float far_plane{ 100.f };		// furthest distance camera can see

		float width_ratio{ 16.f };
		float height_ratio{ 9.f };
		float aspect_ratio{ width_ratio / height_ratio };

		glm::mat4 view() const {
			return glm::lookAt(pos, target, up);
		}

		glm::mat4 projection() const {
			return glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
		}

		static Camera& get() {
			static Camera instance;
			return instance;
		}
	};
}


#endif // PN_PLATFORM_WINDFOWS