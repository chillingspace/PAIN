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
        ~RendererLayer() override = default;

        void onDetach() override { renderer = nullptr; }
        void onAttach() override;
        void onFixedUpdate(AppTiming timing) override {};
        void onUpdate(AppTiming timing) override;

        void onEvent([[maybe_unused]] Event::Event& e) override;
        

    private:

        #ifdef PN_PLATFORM_ANDROID
            std::unique_ptr<AndroidRenderer> renderer;
        #else
        std::unique_ptr<WindowsRenderer> renderer;
        #endif

    };

}
