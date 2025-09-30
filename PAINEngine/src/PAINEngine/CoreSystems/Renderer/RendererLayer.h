#pragma once

#include "pch.h"
#include "PAINEngine/Applications/AppSystem.h"
#include "imgui.h"


//#ifdef PN_PLATFORM_ANDROID
//#include "Android/AndroidRenderer.h"
//#else
#include "Windows/WindowsRenderer.h"
//#endif


namespace PAIN {


    struct SceneObject {
		Mesh* mesh;
		glm::mat4 transform;  // model matrix
	};




	class RendererLayer : public AppSystem {
	public:
		RendererLayer() = default;
		~RendererLayer() = default;

        void onAttach() override;
        void onFixedUpdate(AppTiming timing) override {};
        void onUpdate(AppTiming timing) override;


		static void Submit(Mesh* mesh, const glm::mat4& model);


		void onEvent([[maybe_unused]] Event::Event& e) override;

		ImTextureID getFramebufferTexture() const {
			return (ImTextureID)(intptr_t)fboTexture;
		}
		int getFramebufferWidth() const { return fbWidth; }
		int getFramebufferHeight() const { return fbHeight; }


	private:

// #ifdef PN_PLATFORM_ANDROID
//		std::unique_ptr<AndroidRenderer> renderer;
//#else
		std::unique_ptr<WindowsRenderer> w_renderer;
		

        //static std::vector<SceneObject> s_SubmissionQueue;




		enum MOVE_MODES {
			CAMERA,
			LIGHT,
			NUM_MOVE_MODES
		};
		MOVE_MODES move_mode = CAMERA;

		bool W_KEYDOWN = false;
		bool A_KEYDOWN = false;
		bool S_KEYDOWN = false;
		bool D_KEYDOWN = false;

		bool SPACE_KEYDOWN = false;
		bool LCTRL_KEYDOWN = false;

		bool mouseButtonDown = false;
		float xOffset = 0.0f;
		float yOffset = 0.0f;

		unsigned int fbo = 0;
		unsigned int fboTexture = 0;
		unsigned int rbo = 0;
		int fbWidth = 1280;
		int fbHeight = 720;

	};

}
