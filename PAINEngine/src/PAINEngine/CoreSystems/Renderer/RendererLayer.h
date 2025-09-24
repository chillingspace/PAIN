#ifndef RENDERLAYER_H
#define RENDERLAYER_H

#pragma once

#include "PAINEngine/Applications/AppSystem.h"
#include "PAINEngine/CoreSystems/Renderer/Shader.h"
#include "glm/glm.hpp"

#ifdef PLATFORM_ANDROID
// include android stuff
#endif // PLATFORM_ANDROID

namespace PAIN {

	class RendererLayer {

	public:
		RendererLayer();
		~RendererLayer();

		void onAttach(); // initialize
		void onDetach();
		void onUpdate(); // rendering loop
		void shutdown();

		// Event handler for app layer
		void onEvent(/* Event::Event& e */);

#ifdef PLATFORM_WINDOWS
		bool initGLFW();
		void setWindow(GLFWwindow* window);
		GLFWwindow* getWindow() const { return window; }
#endif // PLATFORM_WINDOWS


	private:
		bool createShaders();
		bool createBuffers();

		GLuint program;
		GLuint vertexShader;
		GLuint fragmentShader;
		GLuint vbo;
		GLuint vao;

		std::unique_ptr<Shader> shader;

		glm::vec3 m_cubePosition;
		glm::vec3 m_cameraPosition;

#ifdef PLATFORM_WINDOWS
		GLFWwindow* window;
#endif // PLATFORM_WINDOWS

		float defaultColor[3];
	};

}























#endif // RENDERERLAYER_H

#pragma once

#include "pch.h"
#include "PAINEngine/Applications/AppSystem.h"


#ifdef PN_PLATFORM_ANDROID
#include "Android/AndroidRenderer.h"
#else

#endif

namespace PAIN {

    class RendererLayer : public AppSystem {
    public:
        RendererLayer() = default;
        ~RendererLayer() = default;

        void onAttach() override;
        void onUpdate() override;

        void onEvent([[maybe_unused]] Event::Event& e) override;
        

    private:

        #ifdef PN_PLATFORM_ANDROID
            std::unique_ptr<AndroidRenderer> renderer;
        #else

        #endif

    };

}
