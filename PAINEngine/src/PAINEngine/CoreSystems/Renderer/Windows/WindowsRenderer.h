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
};


//#ifdef PN_PLATFORM_WINDOWS


#include "pch.h"
#include "../Shader.h"
#include "../Mesh.h"
#include "Applications/AppSystem.h"
#include "Scene/Scene.h"

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
		void RenderMesh(std::shared_ptr<Scene> scene, Mesh* mesh, const glm::mat4& model);
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
		unsigned int rbo = 0;				// depth buffer

		unsigned int final_fbo = 0;
		unsigned int final_texture = 0;		// for imgui
		
		std::unique_ptr<Camera> active_cam = nullptr;
		std::unique_ptr<Shader> pbr_shader = nullptr;
		std::unique_ptr<Shader> geometry_shader = nullptr;
		std::unique_ptr<Shader> floor_shader = nullptr;
		std::unique_ptr<Shader> passthrough_shader = nullptr;

		std::unique_ptr<Mesh> m_mesh = nullptr;

		void _initDeferredShadingBuffers();
	};
}


#endif // PN_PLATFORM_WINDFOWS


namespace PAIN {
	
}

//#endif // __WINDOWS_RENDERER_H__