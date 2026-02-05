/*****************************************************************//**
 * \file   HapticsFactory.cpp
 * \brief  Factory implementation for platform-specific haptics
 *
 * \author PAIN Engine
 * \date   2025
 *********************************************************************/

#include "pch.h"
#include "Haptics.h"

#ifdef PN_PLATFORM_ANDROID
    #include "Android/AndroidHaptics.h"
#else
    #include "Windows/WindowsHaptics.h"
#endif

namespace PAIN {
    namespace Haptics {

        Haptics* Haptics::create(void* app) {
#ifdef PN_PLATFORM_ANDROID
            return new AndroidHaptics(app);
#else
            return new WindowsHaptics(app);
#endif
        }

    }
}
