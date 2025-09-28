
#ifdef PN_PLATFORM_WINDOWS

#include "pch.h"
#include "sLoader.h"

namespace PAIN {
    namespace Loader {

        void PAIN::Loader::Service::onAttach() {
            // Register texture loader
            registerLoader(Types::Texture, [](const std::filesystem::path& path) -> std::shared_ptr<void> {
                // TODO: replace with actual texture loading
                //return std::make_shared<Texture>(path.string());
                return 0;
            });
            addValidExtension(".png", Types::Texture);
            addValidExtension(".jpg", Types::Texture);

            // Register model loader
            registerLoader(Types::Model, [](const std::filesystem::path& path) -> std::shared_ptr<void> {
                // TODO: replace with actual model loading
                //return std::make_shared<Model>(path.string());
                return 0;
            });
            addValidExtension(".obj", Types::Model);
            addValidExtension(".fbx", Types::Model);

            // Register font loader
            registerLoader(Types::Font, [](const std::filesystem::path& path) -> std::shared_ptr<void> {
                // TODO: replace with actual font loading
                //return std::make_shared<Font>(path.string());
                return 0;
            });
            addValidExtension(".ttf", Types::Font);

            // You can keep going for Audio, Script, etc.
        }


        void Service::registerLoader(Types type, LoaderFunc loader) {
            if (loaders.find(type) != loaders.end()) {
                throw std::runtime_error("Loader already registered for this type.");
            }
            loaders[type] = loader;
        }

        std::shared_ptr<void> Service::load(Types type, const std::filesystem::path& path) {
            auto it = loaders.find(type);
            if (it == loaders.end()) {
                throw std::runtime_error("No loader registered for this asset type.");
            }
            return it->second(path);
        }

        void Service::addValidExtension(const std::string& ext, Types type) {
            ext_map[ext] = type;
        }

        Types Service::getTypeFromExtension(const std::filesystem::path& path) const {
            auto ext = path.extension().string();
            auto it = ext_map.find(ext);
            if (it != ext_map.end()) {
                return it->second;
            }
            return Types::None;
        }

    } // namespace Loader
} // namespace PAIN

#endif