#pragma once

#ifndef C_SCRIPT_H
#define C_SCRIPT_H

#include "pch.h"
#include <refl.hpp>

#include "PAINEngine/CoreSystems/Assets/sAssets.h"
#include "LayeredSystems/LevelEditor/EditorAttributes.h"

namespace PAIN {

    struct Script {
        Assets::GUID script_asset;
        bool enabled = true; // active or not
        bool loaded = false; // runtime flag
    };

} 


REFL_TYPE(PAIN::Script)
    REFL_FIELD(script_asset,
        PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Script),   
        PAIN::Editor::Attributes::DisplayName("Script Asset"),
        PAIN::Editor::Attributes::Tooltip("Select a Lua script"))
    REFL_FIELD(enabled)
    //REFL_FIELD(loaded)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Script>);

#endif
