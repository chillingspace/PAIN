#pragma once

#include "pch.h"
// Somehow, if i dont include this, refl macro cannot be foundl, even tho is in pch...
#include "refl.hpp"
#include "LayeredSystems/LevelEditor/EditorAttributes.h"

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/

	struct ModelRenderer {
		std::vector<std::string> model_paths_storage;
		Assets::GUID selected_model;
		ModelRenderer() = default;
		ModelRenderer(Assets::GUID const& id) : selected_model{ id } {}
	};
}


REFL_TYPE(PAIN::ModelRenderer)
REFL_FIELD(selected_model,
	PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Model),
	PAIN::Editor::Attributes::DisplayName("Model Asset"),
	PAIN::Editor::Attributes::Tooltip("Select a 3D model asset"))
REFL_END

