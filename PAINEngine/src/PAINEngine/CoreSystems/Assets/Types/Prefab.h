#pragma once

#ifndef ASSETS_PREFAB_HPP
#define ASSETS_PREFAB_HPP

#include "AssetTypes.h"

namespace PAIN {
namespace Prefab {

// Prefab asset class
struct PrefabAsset : Assets::IAsset {
  std::string prefabName;
  Assets::GUID rootEntityGUID;
  std::vector<nlohmann::json> entities;

  // Custom collider shapes for this prefab (optional)
  // When prefab is instantiated, these become the CompoundCollider component
  std::vector<ColliderShape> defaultColliderShapes;

  PrefabAsset() = default;

  PrefabAsset(const std::string &name, const Assets::GUID &root,
              std::vector<nlohmann::json> &&entities)
      : prefabName(name), rootEntityGUID(root), entities{entities} {}
};
} // namespace Prefab
} // namespace PAIN

#endif
