#pragma once

#ifndef ENTITY_COMP_HPP
#define ENTITY_COMP_HPP

#include "AssetData.h"

namespace PAIN {
	namespace Entity {
        
        //Unique entity GUID
        struct GUID {
            Assets::GUID guid;

            //Serialization flag
            static constexpr bool ShouldSerialize = true;
        };

        //Editor readable entity name
        struct Name {
            std::string name;

            //Serialization flag
            static constexpr bool ShouldSerialize = true;
        };

        //Entity hierachy
        struct Hierarchy {
            Assets::GUID parentGUID;
            std::vector<Assets::GUID> childrenGUIDs;
            int siblingIndex = 0;

            //Serialization flag
            static constexpr bool ShouldSerialize = true;
        };

        //Layer comp
        struct Layer {
            int layer_id = 0;
            int layer_mask = 1;

            // Optional: Store scene layer name for debugging
            std::string layerName = "Default";

            // Check if this layer can interact with another layer
            bool canInteractWith(uint32_t otherLayerMask) const {
                return (layer_mask & otherLayerMask) != 0;
            }

            //Serialization flag
            static constexpr bool ShouldSerialize = true;
        };
	}
}

// Reflection for serialization
REFL_TYPE(PAIN::Entity::GUID)
REFL_FIELD(guid)
REFL_END

REFL_TYPE(PAIN::Entity::Name)
REFL_FIELD(name)
REFL_END

REFL_TYPE(PAIN::Entity::Hierarchy)
REFL_FIELD(parentGUID)
REFL_FIELD(childrenGUIDs)
REFL_FIELD(siblingIndex)
REFL_END

REFL_TYPE(PAIN::Entity::Layer)
REFL_FIELD(layer_id)
REFL_FIELD(layer_mask)
REFL_END

#endif
