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

	struct MeshRenderer {
		//std::shared_ptr<Mesh> mesh;
		uint32_t mesh_id;
	};

#ifdef _DEBUG
	// UI Registration function
	inline void RegisterMeshRendererUI(Editor::Panel::ComponentsPanel& panel) {
		panel.registerCompUIFunc<MeshRenderer>([](Editor::Panel::ComponentsPanel& comp_panel, MeshRenderer& mesh) {
			ImGui::Text("Mesh Renderer");
			ImGui::Separator();
			
			unsigned int step = 1;
			ImGui::InputScalar("Mesh ID", ImGuiDataType_U32, &mesh.mesh_id, &step);

		});

	}

#endif

}


REFL_TYPE(PAIN::MeshRenderer)
REFL_FIELD(mesh_id)
REFL_END

