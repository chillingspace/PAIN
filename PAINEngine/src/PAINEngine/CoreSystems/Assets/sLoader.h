#pragma once

#include "Applications/AppSystem.h"

namespace PAIN {
    namespace Loader {

        enum class Types {
            None,
            Texture,
            Model,
            Font,
            Audio,
            Sound,
            Scene,
            Prefab,
            Grid,
            Script,
            Video,
            Sprite
        };

        using LoaderFunc = std::function<std::shared_ptr<void>(const std::filesystem::path&)>;

        class Service : public AppSystem {
        public:
            Service() = default;
            ~Service() = default;

            void registerLoader(Types type, LoaderFunc loader);
            std::shared_ptr<void> load(Types type, const std::filesystem::path& path);

            void addValidExtension(const std::string& ext, Types type);
            Types getTypeFromExtension(const std::filesystem::path& path) const;

        private:
            std::map<Types, LoaderFunc> loaders;
            std::map<std::string, Types> ext_map; // e.g. ".png" -> Texture
        };

    } // namespace Loader
} // namespace PAIN
