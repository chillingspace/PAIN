#pragma once

#ifndef EDITOR_ATTRIBUTES_HPP
#define EDITOR_ATTRIBUTES_HPP 

#undef max
#undef min

#include "refl.hpp"
#include "CoreSystems/Assets/sAssets.h"

namespace PAIN::Editor::Attributes {

    // Base attribute for asset selection
    struct AssetSelector : refl::attr::usage::field {
        Assets::Type asset_type;

        constexpr explicit AssetSelector(Assets::Type type)
            : asset_type(type) {
        }
    };

    // Attribute for dropdown/combo box
    struct Dropdown : refl::attr::usage::field {
        constexpr Dropdown() = default;
    };

    // Attribute to specify display name
    struct DisplayName : refl::attr::usage::field {
        const char* name;

        constexpr explicit DisplayName(const char* n)
            : name(n) {
        }
    };

    // Attribute for read-only fields
    struct ReadOnly : refl::attr::usage::field {
        constexpr ReadOnly() = default;
    };

    // Attribute for tooltips
    struct Tooltip : refl::attr::usage::field {
        const char* text;

        constexpr explicit Tooltip(const char* t)
            : text(t) {
        }
    };

    // Attribute for value ranges (for sliders)
    struct Range : refl::attr::usage::field {
        float min;
        float max;

        constexpr Range(float min_val, float max_val)
            : min(min_val), max(max_val) {
        }
    };

}

#endif