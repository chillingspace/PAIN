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
        void onFixedUpdate(AppTiming timing) override {};
        void onUpdate(AppTiming timing) override;

        void onEvent([[maybe_unused]] Event::Event& e) override;
        

    private:

        #ifdef PN_PLATFORM_ANDROID
            std::unique_ptr<AndroidRenderer> renderer;
        #else

        #endif

    };

}