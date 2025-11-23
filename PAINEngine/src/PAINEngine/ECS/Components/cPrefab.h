#pragma once

#ifndef PREFAB_COMP_HPP
#define PREFAB_COMP_HPP

#include "AssetData.h"

namespace PAIN {
    namespace Prefab {

        struct PrefabInstance {
            Assets::GUID sourcePrefabGUID;
            Assets::GUID instanceRootGUID;
            Assets::GUID correspondingPrefabEntityGUID;

            //Prefab instance comp overrides
            std::unordered_map<std::string, nlohmann::json> componentOverrides;

            PrefabInstance() = default;
            PrefabInstance(const Assets::GUID& prefabGUID, const Assets::GUID& rootGUID)
                : sourcePrefabGUID(prefabGUID), instanceRootGUID(rootGUID) {
            }

            //Serialization flag
            static constexpr bool ShouldSerialize = true;
        };
    }
}

REFL_TYPE(PAIN::Prefab::PrefabInstance)
REFL_FIELD(sourcePrefabGUID)
REFL_FIELD(instanceRootGUID)
REFL_FIELD(correspondingPrefabEntityGUID)
REFL_FIELD(componentOverrides)
REFL_END

#endif
