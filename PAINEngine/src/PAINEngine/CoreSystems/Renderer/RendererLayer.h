#pragma once

#include "pch.h"
#include "PAINEngine/Applications/AppSystem.h"


#ifdef PN_PLATFORM_ANDROID
#include "Android/AndroidRenderer.h"
#else
#include "Windows/WindowsRenderer.h"
#endif

namespace PAIN {

	class RendererLayer : public AppSystem {
	public:
		RendererLayer() = default;
		~RendererLayer() = default;

        void onAttach() override;
        void onFixedUpdate(AppTiming timing) override {};
        void onUpdate(AppTiming timing) override;

		void onEvent([[maybe_unused]] Event::Event& e) override;


	private:

#ifdef PN_PLATFORM_ANDROID
		std::unique_ptr<AndroidRenderer> renderer;
#else
		std::unique_ptr<WindowsRenderer> w_renderer;
#endif

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

	};

}
