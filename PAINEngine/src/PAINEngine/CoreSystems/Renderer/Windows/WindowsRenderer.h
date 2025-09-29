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

#ifndef __WINDOWS_RENDERER_H__
#define __WINDOWS_RENDERER_H__

#include "../Light.h"
#include "../Material.h"

namespace PAIN {
	extern Material material;
	extern Light light;
};


//#ifdef PN_PLATFORM_WINDOWS


#include "pch.h"
#include "../Shader.h"
#include "../Mesh.h"
#include "../../../Applications/AppSystem.h"

namespace PAIN {
	static constexpr float ao = 1.f;		// ambient occlusion	(1 = no occlusion)

	class WindowsRenderer {
	public:
		WindowsRenderer();
		~WindowsRenderer();

		void Init();
		void Render();
		void RenderMesh(Mesh* mesh, const glm::mat4& model);
		void Clear();
		void Cleanup();

		/*bool InitGLFW();
		void SetWindow(GLFWwindow* window);
		GLFWwindow* GetWindow() const { return window; };*/

	private:
		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ebo = 0;

		unsigned int empty_vao = 0;

		unsigned int fbo = 0;

		std::unique_ptr<Shader> m_shader = nullptr;
		std::unique_ptr<Shader> sphere_shader = nullptr;
		std::unique_ptr<Shader> floor_shader = nullptr;

		std::unique_ptr<Mesh> m_mesh = nullptr;

	};
}


#endif // PN_PLATFORM_WINDFOWS


namespace PAIN {
	class Camera {
	private:
		Camera() = default;
		~Camera() = default;
	public:
		enum MOVE_MODES {
			FPS,
			ORBIT_ORIGIN,
			NUM_MOVE_MODES,
		};

		MOVE_MODES move_mode = ORBIT_ORIGIN;

		float speed = 15.f;

		float sensitivity = 0.1f;

		glm::vec3 pos{ 0.f, 5.f, 7.f };
		glm::vec3 forward{ -glm::normalize(pos) };
		glm::vec3 up{ 0.f, 1.f, 0.f };

		float fov{ 90.f };
		float near_plane{ 0.1f };		// closest distance camera can see
		float far_plane{ 100.f };		// furthest distance camera can see

		float width_ratio{ 16.f };
		float height_ratio{ 9.f };
		float aspect_ratio{ width_ratio / height_ratio };

		// temp
		glm::mat4 model() const {
			glm::mat4 m = glm::mat4(1.f);
			m = glm::translate(m, glm::vec3(0.f, 1.f, 0.f));
			return m;
		}

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

//#endif // __WINDOWS_RENDERER_H__