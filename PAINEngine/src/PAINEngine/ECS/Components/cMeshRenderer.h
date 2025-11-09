#pragma once

#include "pch.h"
// Somehow, if i dont include this, refl macro cannot be foundl, even tho is in pch...
#include "refl.hpp"
#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h"

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/

	struct ModelRenderer {
		uint32_t mesh_id{};
	};

#ifdef _DEBUG
	// UI Registration function
	inline void RegisterMeshRendererUI(Editor::Panel::ComponentsPanel& panel) {
		panel.registerCompUIFunc<ModelRenderer>([](Editor::Panel::ComponentsPanel& comp_panel, ModelRenderer& mesh_id) {
			ImGui::Text("Model Renderer");
			ImGui::Separator();
			
			unsigned int step = 1;
			ImGui::InputScalar("Mesh ID", ImGuiDataType_U32, &mesh_id, &step);

		});

	}

#endif

}


REFL_TYPE(PAIN::ModelRenderer)
REFL_FIELD(mesh_id)
REFL_END

