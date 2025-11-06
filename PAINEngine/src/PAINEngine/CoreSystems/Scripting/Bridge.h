#pragma once
#include <memory>

struct IEngineAPI;

namespace PAIN {
    namespace ECS { class Controller; }
    namespace MetaData { class Service; }
    namespace Assets { class Manager; }
    //namespace Audio { class Audio; }
    namespace Path { class Path; }
}

namespace PAIN::Scripting {

    struct EngineWiring {
        std::shared_ptr<IEngineAPI> api;

        EngineWiring();
        ~EngineWiring();

        EngineWiring(EngineWiring&&) noexcept;
        EngineWiring& operator=(EngineWiring&&) noexcept;

        EngineWiring(const EngineWiring&) = delete;
        EngineWiring& operator=(const EngineWiring&) = delete;
    };

    EngineWiring CreateMinimalEngineForAndroid(); // test ver

    EngineWiring CreateProductionEngine(
        PAIN::ECS::Controller& ecs,
        PAIN::MetaData::Service& meta,
        PAIN::Assets::Manager* assets,
        //PAIN::Audio::Audio* audio,
        PAIN::Path::Path* fs
    );

}  // namespace PAIN::Scripting
