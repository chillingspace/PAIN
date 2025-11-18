#pragma once

#include "pch.h"

#include "LayeredSystems/LevelEditor/EditorAttributes.h"

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/

	struct ModelRenderer {
		std::vector<std::string> model_paths_storage;
		Assets::GUID selected_model;

		std::vector<std::string> diff_tex_paths_storage;
		Assets::GUID selected_diff_tex;

		std::vector<std::string> ao_tex_paths_storage;
		Assets::GUID selected_ao_tex;

		// Material
		glm::vec3 baseColor{ 1.f, 1.f, 1.f };
		float metallic{ 0.f };
		float roughness{ 1.f };
		
		ModelRenderer() = default;
		ModelRenderer(Assets::GUID const& id) : selected_model{ id } {};
		ModelRenderer(Assets::GUID const& id, Assets::GUID const& diff_id, Assets::GUID const& ao_id, glm::vec3 base_color, float metallic, float roughness) :
			selected_model{ id }, baseColor{ base_color }, metallic{ metallic }, roughness{ roughness }, selected_diff_tex{ diff_id }, selected_ao_tex{ ao_id } {
		};
	};
}


REFL_TYPE(PAIN::ModelRenderer)
REFL_FIELD(selected_model,
	PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Model),
	PAIN::Editor::Attributes::DisplayName("Model Asset"),
	PAIN::Editor::Attributes::Tooltip("Select a 3D model asset"))
REFL_FIELD(selected_diff_tex,
	PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
	PAIN::Editor::Attributes::DisplayName("Diffuse Texture"),
	PAIN::Editor::Attributes::Tooltip("Select a diffuse Texture"))
REFL_FIELD(selected_ao_tex,
	PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
	PAIN::Editor::Attributes::DisplayName("AO Texture"),
	PAIN::Editor::Attributes::Tooltip("Select a AO Texture"))
REFL_FIELD(baseColor,
	PAIN::Editor::Attributes::DisplayName("Base Color"),
	PAIN::Editor::Attributes::Tooltip("Base color of the material"))
REFL_FIELD(metallic,
	PAIN::Editor::Attributes::DisplayName("Metallic"),
	PAIN::Editor::Attributes::Tooltip("Metallic factor of the material"))
REFL_FIELD(roughness,
	PAIN::Editor::Attributes::DisplayName("Roughness"),
	PAIN::Editor::Attributes::Tooltip("Roughness factor of the material"))
REFL_END

