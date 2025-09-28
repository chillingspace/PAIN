/**
 * @file WindowsRenderer.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-09-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */


#pragma once

#ifdef PN_PLATFORM_WINDOWS

#include "pch.h"
#include "../Shader.h"
#include "../Mesh.h"
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

		struct Material {
			float rough;
			float metal;
			glm::vec3 color;
		};

		struct Light {
			glm::vec3 position;
			glm::vec3 L_intensity;
		};

		Material material = {
			0.1f,		// 0.1 -> smooth, 1 -> rough
			0.3f,
			{0.5f,0.5f,0.5f}
		};

		Light light = {
			{0.f, 0.f, 0.f},	// position
			{0.1f, 0.1f, 0.1f}					// intensity
		};

		static constexpr float ao = 1.f;		// ambient occlusion	(1 = no occlusion)

		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ebo = 0;

		std::unique_ptr<Shader> m_shader = nullptr;
		std::unique_ptr<Mesh> m_mesh = nullptr;
	};

	class Camera {
	private:
		Camera() = default;
		~Camera() = default;
	public:
		float speed = 15.f;

		glm::vec3 pos{ 0.f, 0.f, 3.f };
		glm::vec3 forward{ 0.f, 0.f, -1.f };
		glm::vec3 up{ 0.f, 1.f, 0.f };

		float fov{ 90.f };
		float near_plane{ 0.1f };		// closest distance camera can see
		float far_plane{ 100.f };		// furthest distance camera can see

		float width_ratio{ 16.f };
		float height_ratio{ 9.f };
		float aspect_ratio{ width_ratio / height_ratio };

		glm::mat4 view() const {
			return glm::lookAt(pos, pos + forward, up);
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