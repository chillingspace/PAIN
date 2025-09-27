#include "pch.h"
#include "sLoader.h"

namespace PAIN {
    namespace Loader {

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
