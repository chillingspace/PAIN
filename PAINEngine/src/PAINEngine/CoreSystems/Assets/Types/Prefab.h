#pragma once

#ifndef ASSETS_PREFAB_HPP
#define ASSETS_PREFAB_HPP

#include "AssetTypes.h"

namespace PAIN {
	namespace Assets {
		namespace Prefabs {

            //Prefab asset class
            struct PrefabAsset : IAsset {
                Assets::GUID prefabGUID;
                std::string prefabName;
                Assets::GUID rootEntityGUID;
                std::vector<Assets::GUID> entityGUIDs;

                PrefabAsset() = default;

                PrefabAsset(const Assets::GUID& guid, const std::string& name, const Assets::GUID& root)
                    : prefabGUID(guid), prefabName(name), rootEntityGUID(root) {
                }
            };

		}
	}
}

#endif
