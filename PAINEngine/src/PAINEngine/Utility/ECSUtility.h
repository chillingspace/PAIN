#pragma once

#include "pch.h"

namespace PAIN
{
    namespace ECS {
        namespace Utility {
            static std::string convertTypeString(std::string&& str_type) {
		        return str_type.substr(str_type.find_first_not_of(':', str_type.find_first_of(':')), str_type.size() - str_type.find_first_not_of(':', str_type.find_first_of(':')));
            }
	
        }
    }
} // namespace PAIN
