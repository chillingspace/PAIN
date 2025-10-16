#pragma once

#include <CoreSystems/Renderer/Mesh.h>

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/

	struct MeshRenderer {
		std::shared_ptr<Mesh> mesh;
	};



}

