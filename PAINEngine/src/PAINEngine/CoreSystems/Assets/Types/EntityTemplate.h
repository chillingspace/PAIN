#pragma once

#ifndef ASSETS_ENTITY_TEMPLATE_HPP
#define ASSETS_ENTITY_TEMPLATE_HPP

#include "AssetTypes.h"

namespace PAIN {
    namespace EntityTemplate {

        // Entity Template asset
        struct TemplateAsset : Assets::IAsset {
            std::string templateName;
            nlohmann::json componentData;  // All components serialized
            std::vector<std::string> tags;  // Optional tags for categorization
            
            TemplateAsset() = default;
            
            TemplateAsset(const std::string& name, nlohmann::json&& components)
                : templateName(name), componentData(std::move(components)) {
            }
            
            TemplateAsset(const std::string& name, nlohmann::json&& components, std::vector<std::string>&& templateTags)
                : templateName(name), componentData(std::move(components)), tags(std::move(templateTags)) {
            }
        };
    }
}

#endif
