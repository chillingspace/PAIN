#pragma once

#ifndef ASSETS_PREFAB_HPP
#define ASSETS_PREFAB_HPP

#include "AssetTypes.h"
#include "ECS/Components/cCompoundCollider.h"

namespace PAIN {
namespace Prefab {

// Prefab asset class
struct PrefabAsset : Assets::IAsset {
	std::string prefabName;
	Assets::GUID rootEntityGUID;
	std::vector<nlohmann::json> entities;

	// Default compound collider shapes for this prefab
	std::vector<ColliderShape> defaultColliderShapes;

	PrefabAsset() = default;

	PrefabAsset(const std::string& name, const Assets::GUID& root,
							std::vector<nlohmann::json> &&entities)
			: prefabName(name), rootEntityGUID(root), entities{entities} {}
};
} // namespace Prefab
} // namespace PAIN

#endif
