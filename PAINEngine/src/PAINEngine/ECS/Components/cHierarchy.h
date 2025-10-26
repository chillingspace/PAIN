/*****************************************************************//**
 * \file   cHierarchy.h
 * \brief  Hierarchy component for parent-child entity relationships
 *
 * \author [Your Name], [Your ID], [Your Email] (100%)
 * \co-author
 * \date   October 2025
 * All content © 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#ifndef C_HIERARCHY_H
#define C_HIERARCHY_H

#include <entt/entt.hpp>
#include <vector>
#include "Jolt/Jolt.h"
#include <Jolt/Core/Factory.h>          
#include <Jolt/RegisterTypes.h>   

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

		/**
		 * \brief Check if this entity is a root entity (no parent)
		 * \return true if parent is null, false otherwise
		 */
		bool isRoot() const {
			return parent == entt::null;
		}

		/**
		 * \brief Check if this entity has any children
		 * \return true if children vector is not empty
		 */
		bool hasChildren() const {
			return !children.empty();
		}

		/**
		 * \brief Get the number of direct children
		 * \return Number of children
		 */
		size_t getChildCount() const {
			return children.size();
		}
	};

} // namespace PAIN

#endif // C_HIERARCHY_H
