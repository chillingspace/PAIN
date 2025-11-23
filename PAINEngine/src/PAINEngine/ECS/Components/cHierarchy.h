/*****************************************************************//**
 * \file   cHierarchy.h
 * \brief  Definition of animation system states
 *
 * \author Nicole Esther Lee, 2301544, [lee.n@digipen.edu] (100%)
 * \co-author
 * \date   24 October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#ifndef C_HIERARCHY_H
#define C_HIERARCHY_H

#include "pch.h"

namespace PAIN {

	/*****************************************************************//**
	* Hierarchy Component
	* Used to establish parent-child relationships between entities
	* in the scene hierarchy (similar to Unity's transform hierarchy)
	*********************************************************************/

	struct Hierarchy {

		/**
		 * \brief Parent entity in the hierarchy
		 * Set to entt::null if this entity is a root (has no parent)
		 */
		entt::entity parent = entt::null;

		/**
		 * \brief List of child entities
		 * Contains all direct children of this entity in the hierarchy
		 */
		std::vector<entt::entity> children;
	};

} // namespace PAIN

REFL_TYPE(PAIN::Hierarchy)
REFL_FIELD(parent)
REFL_FIELD(children)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Hierarchy>);

#endif // C_HIERARCHY_H
