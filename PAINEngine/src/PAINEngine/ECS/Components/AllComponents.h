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
#include "Core.h"

namespace PAIN {
    // All gameplay components (NOT metadata components)
    /*
    * Some component pipeline to take note for now: 
    * 1. When creating a component, follow one of the component files to add in the seri stuff
    * 2. Include your component in Core.h
    * 3. Add in your component to this file 
    * 4. Register your component in controller.cpp
    * 5. If IMGUI UI is needed add it in components panel (REFL cannot work on enum)
    */
    using AllGameplayComponents = std::tuple <
        //Entity components
        Entity::GUID,
        Entity::Name,
        Entity::Hierarchy,
        Entity::Layer,

        //prefan comps
        Prefab::PrefabInstance,

        // Metadata components
        MetaData::Tag,
        MetaData::EditorVisible,

        // Gameplay
        Cam,
        LocalTransform,
        WorldTransform,
        ModelRenderer,
        Lighting,
        Physics::RigidBody3D,
        Collision::Collider,
        Joint,
        BoundingVolume,
        Audio::AudioSource,
        Scripts,

        // UI comps
        UIRectTransform,
        UIButton,
        UIElement,
        UICanvas,
        UIAnimation,
        UIText,
        // AI
        //AI::Blackboard,
        AI::Controller,
        AI::Sensors,
        AI::NavAgent,
        AI::Steering
        //AI::CommandQueue
    >;

    template<typename T>
    constexpr const char* getComponentName() {
        //Entity components
        if constexpr (std::is_same_v<T, Entity::GUID>) return "GUID";
        if constexpr (std::is_same_v<T, Entity::Name>) return "Name";
        if constexpr (std::is_same_v<T, Entity::Hierarchy>) return "Hierarchy";
        if constexpr (std::is_same_v<T, Entity::Layer>) return "Layer";

        //Prefab components
        if constexpr (std::is_same_v<T, Prefab::PrefabInstance>) return "PrefabInstance";

        // Metadata components
        else if constexpr (std::is_same_v<T, MetaData::Tag>) return "Tag";
        else if constexpr (std::is_same_v<T, MetaData::EditorVisible>) return "EditorVisible";

        // Gameplay components
        else if constexpr (std::is_same_v<T, Cam>) return "Camera";
        else if constexpr (std::is_same_v<T, LocalTransform>) return "LocalTransform";
        else if constexpr (std::is_same_v<T, WorldTransform>) return "WorldTransform";
        else if constexpr (std::is_same_v<T, ModelRenderer>) return "ModelRenderer";
        else if constexpr (std::is_same_v<T, Lighting>) return "Lighting";
        else if constexpr (std::is_same_v<T, Physics::RigidBody3D>) return "RigidBody3D";
        else if constexpr (std::is_same_v<T, Collision::Collider>) return "Collider";
        else if constexpr (std::is_same_v<T, Joint>) return "Joint";
        else if constexpr (std::is_same_v<T, BoundingVolume>) return "BoundingVolume";
        else if constexpr (std::is_same_v<T, Audio::AudioSource>) return "AudioSource";
        else if constexpr (std::is_same_v<T, Scripts>) return "Scripts";

        // UI comps
		else if constexpr (std::is_same_v<T, UIRectTransform>) return "UIRectTransform";
        else if constexpr (std::is_same_v<T, UIButton>) return "UIButton";
        else if constexpr (std::is_same_v<T, UIElement>) return "UIElement";
        else if constexpr (std::is_same_v<T, UICanvas>) return "UICanvas";
        else if constexpr (std::is_same_v<T, UIAnimation>) return "UIAnimation";
        else if constexpr (std::is_same_v<T, UIText>) return "UIText";
        // AI components
        //else if constexpr (std::is_same_v<T, AI::Blackboard>) return "AIBlackboard";
        else if constexpr (std::is_same_v<T, AI::Controller>) return "AIController";
        else if constexpr (std::is_same_v<T, AI::Sensors>) return "AISensors";
        

        else return "Unknown";
    }
}

    