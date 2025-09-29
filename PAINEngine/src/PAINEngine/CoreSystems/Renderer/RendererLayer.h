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
        void onUpdate(float dt) override;

        void onEvent([[maybe_unused]] Event::Event& e) override;
        

    private:

        #ifdef PN_PLATFORM_ANDROID
            std::unique_ptr<AndroidRenderer> renderer;
        #else
        std::unique_ptr<WindowsRenderer> w_renderer;
        #endif

		bool W_KEYDOWN = false;
		bool A_KEYDOWN = false;
		bool S_KEYDOWN = false;
		bool D_KEYDOWN = false;

		bool mouseButtonDown = false;
		float xOffset = 0.0f;
		float yOffset = 0.0f;

    };

}
