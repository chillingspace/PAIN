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

#include "Applications/Application.h"
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Scene/Camera.h"

#include "../Light.h"
#include "../Material.h"

namespace PAIN {
	extern Material material;
};


//#ifdef PN_PLATFORM_WINDOWS


#include "pch.h"
#include "../Shader.h"
#include "../Mesh.h"
#include "Applications/AppSystem.h"
#include "CoreSystems/Scene/Scene.h"

namespace PAIN {
	static constexpr float ao = 1.f;		// ambient occlusion	(1 = no occlusion)

	class WindowsRenderer {
	private:
		WindowsRenderer();
		~WindowsRenderer();

	public:

		static WindowsRenderer& get() {
			static WindowsRenderer instance;
			return instance;
		}

		void Init();
		void Render(std::shared_ptr<Scene> scene);
		void RenderGeometry(std::shared_ptr<Scene> scene, Mesh* mesh, const glm::mat4& model);
		void RenderGeometryShadows(Mesh* mesh, const glm::mat4& model, const Light& light);
		//void RenderScene(std::shared_ptr<Scene> scene);
		void Cleanup();

		/*bool InitGLFW();
		void SetWindow(GLFWwindow* window);
		GLFWwindow* GetWindow() const { return window; };*/

		unsigned int getFinalFbo() const {
			return final_fbo;
		}

		unsigned int getFinalTexture() const {
			return final_texture;
		}

	private:
		unsigned int vao = 0;
		unsigned int vbo = 0;
		unsigned int ebo = 0;

		unsigned int empty_vao = 0;

		unsigned int passthrough_vao = 0;
		unsigned int passthrough_vbo = 0;

		unsigned int ds_fbo = 0;			// deferred shading framebuffer
		unsigned int pos_texture = 0;
		unsigned int col_texture = 0;
		unsigned int norm_texture = 0;
		unsigned int material_properties_texture = 0;		// 2D to store roughness, metallic properties
		unsigned int ds_rbo = 0;				// depth buffer

		//unsigned int shadow_fbo = 0;
		//unsigned int shadow_texture = 0;					// shadow map

		unsigned int final_fbo = 0;
		unsigned int final_texture = 0;		// for imgui
		
		std::unique_ptr<Camera> active_cam = nullptr;

		// for easy access to clear memory
		std::array<unsigned int*, 2> fbos{ 
			&ds_fbo, 
			//&shadow_fbo, 
			&final_fbo 
		};
		std::array<unsigned int*, 1> rbos{ &ds_rbo };
		std::array<unsigned int*, 4> texs{
			&pos_texture,
			&col_texture,
			&norm_texture,
			&material_properties_texture,
			//&shadow_texture
		};

		std::unique_ptr<Shader> pbr_shader = nullptr;
		std::unique_ptr<Shader> geometry_shader = nullptr;
		std::unique_ptr<Shader> floor_shader = nullptr;
		std::unique_ptr<Shader> passthrough_shader = nullptr;
		std::unique_ptr<Shader> shadow_shader = nullptr;

		std::unique_ptr<Mesh> m_mesh = nullptr;

		/**
		 * .
		 *
		 * \param tex
		 * \param num_i channels
		 * \param gl_color_attachment THIS IS NOT YOUR NORMAL ID. USE GL_ATTACHMENT`n` HERE.
		 */
		void _createDeferredShadingBuffer(unsigned int& tex, int num_channels, int gl_color_attachment);
		void _initDeferredShadingBuffers();
	};
}


#endif // PN_PLATFORM_WINDFOWS


// namespace PAIN {
// 	class Camera {
// 	private:
// 		Camera() {
// 			width_ratio = winWidth;
// 			height_ratio = winHeight;
// 			aspect_ratio = width_ratio / height_ratio;
// 		};
// 		~Camera() = default;
// 	public:
// 		enum MOVE_MODES {
// 			FPS,
// 			ORBIT_ORIGIN,
// 			NUM_MOVE_MODES,
// 		};

// 		MOVE_MODES move_mode = FPS;

// 		float speed = 15.f;

// 		float sensitivity = 0.1f;

// 		glm::vec3 pos{ 0.f, 2.f, 4.f };
// 		glm::vec3 forward{ -glm::normalize(pos) };
// 		glm::vec3 up{ 0.f, 1.f, 0.f };

// 		float fov{ 90.f };
// 		float near_plane{ 0.1f };		// closest distance camera can see
// 		float far_plane{ 100.f };		// furthest distance camera can see

// 		float width_ratio{ 16.f };
// 		float height_ratio{ 9.f };
// 		float aspect_ratio{ width_ratio / height_ratio };

// 		// temp
// 		glm::mat4 model() const {
// 			glm::mat4 m = glm::mat4(1.f);
// 			m = glm::translate(m, glm::vec3(0.f, 1.f, 0.f));
// 			return m;
// 		}

// 		glm::mat4 view() const {
// 			return glm::lookAt(pos, pos + forward, up);
// 		}

// 		glm::mat4 projection() const {
// 			return glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
// 		}

// 		static Camera& get() {
// 			static Camera instance;
// 			return instance;
// 		}
// 	};
// }

// //#endif // __WINDOWS_RENDERER_H__