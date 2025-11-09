#pragma once

#include "pch.h"

namespace PAIN
{
    namespace ECS {
        namespace Utility {
            static std::string convertTypeString(std::string&& str_type) {
                size_t first_colon = str_type.find_first_of(':');

                // Check if colon mesh_id
                if (first_colon == std::string::npos) {
                    return str_type; // No colon found, return original string
                }

                size_t start_pos = str_type.find_first_not_of(':', first_colon);

                // Check if any non-colon character mesh_id after the colon
                if (start_pos == std::string::npos) {
                    return ""; // Only colons after first colon, return empty string
                }

                // Safely extract substring
                return str_type.substr(start_pos);
            }
	
        }
    }
} // namespace PAIN
