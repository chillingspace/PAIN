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

#endif
