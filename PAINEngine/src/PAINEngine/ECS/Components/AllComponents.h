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
#include "cAudioSource.h"
#include "cBoundingVolume.h"
#include "cScript.h"
#include "cEntity.h"

namespace PAIN {
    // All gameplay components (NOT metadata components)
    /*
    * Some component pipeliens to take note for now: 
    * 1. When creating a component, follow one of the component files to add in the seri stuff
    * 2. Add in this file your component
    * 3. Register your component in controller.cpp
    */
    using AllGameplayComponents = std::tuple <
        //Entity components
        Entity::GUID,
        Entity::Name,
        Entity::Hierarchy,

        // Metadata components
        MetaData::EntityName,
        MetaData::Tag,
        MetaData::EditorVisible,
        MetaData::Relation,
        MetaData::Group,
        Hierarchy,

        // Gameplay
        Transform,
        ModelRenderer,
        Lighting,
        Physics::RigidBody3D,
        Collision::Collider,
        Joint,
        BoundingVolume,
        Audio::AudioSource,
        Script
    >;

    template<typename T>
    constexpr const char* getComponentName() {
        //Entity components
        if constexpr (std::is_same_v<T, Entity::GUID>) return "GUID";
        if constexpr (std::is_same_v<T, Entity::Name>) return "Name";
        if constexpr (std::is_same_v<T, Entity::Hierarchy>) return "Hierarchy";

        // Metadata components
        if constexpr (std::is_same_v<T, MetaData::EntityName>) return "EntityName";
        else if constexpr (std::is_same_v<T, MetaData::Tag>) return "Tag";
        else if constexpr (std::is_same_v<T, MetaData::EditorVisible>) return "EditorVisible";
        else if constexpr (std::is_same_v<T, MetaData::Relation>) return "Relation";
        else if constexpr (std::is_same_v<T, MetaData::Group>) return "Group";
        else if constexpr (std::is_same_v<T, Hierarchy>) return "Hierarchy";

        // Gameplay components
        else if constexpr (std::is_same_v<T, Transform>) return "Transform";
        else if constexpr (std::is_same_v<T, ModelRenderer>) return "ModelRenderer";
        else if constexpr (std::is_same_v<T, Lighting>) return "Lighting";
        else if constexpr (std::is_same_v<T, Physics::RigidBody3D>) return "RigidBody3D";
        else if constexpr (std::is_same_v<T, Collision::Collider>) return "Collider";
        else if constexpr (std::is_same_v<T, Joint>) return "Joint";
        else if constexpr (std::is_same_v<T, BoundingVolume>) return "BoundingVolume";
        else if constexpr (std::is_same_v<T, Audio::AudioSource>) return "AudioSource";
        else if constexpr (std::is_same_v<T, Script>) return "Script";
        else return "Unknown";
    }
}

    