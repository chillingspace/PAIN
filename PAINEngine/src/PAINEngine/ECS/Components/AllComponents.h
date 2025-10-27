/*****************************************************************//**
 * \file   AllComponents.h
 * \brief  All physics data components
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

// To include all components excluding metadata
#include "cTransform.h"
#include "cHierarchy.h"
#include "cMeshRenderer.h"
#include "cLight.h"
#include "cPhysics.h"
#include "cMetadata.h"

namespace PAIN {
    // All gameplay components (NOT metadata components)
    /*
    * Some component pipeliens to take note for now: 
    * 1. When creating a component, follow one of the component files to add in the seri stuff
    * 2. Add in this file your component
    * 3. Register your component in controller.cpp
    */
    using AllGameplayComponents = std::tuple <
        Transform,
        Hierarchy,
        MeshRenderer,
        Lighting,
        Physics::RigidBody3D,
        Collision::Collider,
        Joint
    >;

    template<typename T>
    constexpr const char* getComponentName() {
        if constexpr (std::is_same_v<T, Transform>) return "Transform";
        else if constexpr (std::is_same_v<T, Hierarchy>) return "Hierarchy";
        else if constexpr (std::is_same_v<T, MeshRenderer>) return "MeshRenderer";
        else if constexpr (std::is_same_v<T, Lighting>) return "Lighting";
        else if constexpr (std::is_same_v<T, Physics::RigidBody3D>) return "RigidBody3D";
        else if constexpr (std::is_same_v<T, Collision::Collider>) return "Collidor";
        else if constexpr (std::is_same_v<T, Joint>) return "Joint";
        else return "Unknown";
    }
}

    