#pragma once

#include <bitset>

namespace PAIN {
    namespace ECS {
        namespace Entity {
            //Entity Type
            using Type = uint16_t;

            //Max number of entities to be created at a single point
            const Type MAX = 3000;

            //iNVALID COMP TO BE ASSIGNED WITH A HUGE NUMBER
            const Type INVALID = UINT16_MAX;
        }

        namespace Component {
            //Component Signature Type
            using Type = uint8_t;

            //Max components to be stored in a signature
            const Type MAX = INT8_MAX;

            //Nested components signature
            using Signature = std::bitset<MAX>;
        }
    }

}