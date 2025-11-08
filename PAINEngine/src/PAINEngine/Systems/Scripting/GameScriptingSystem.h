#pragma once

#include "Applications/AppSystem.h"
#include "PAINEngine/CoreSystems/Scripting/luaManager.h"
#include "PAINEngine/CoreSystems/Scripting/IEngineAPI.h"
#include "ECS/System/ISystem.h"


namespace PAIN {

    class Services;

    namespace Scripting {

        class GameScriptingSystem : public ECS::System::ISystem {
        public:

            explicit GameScriptingSystem(std::shared_ptr<Services> svc); 
            ~GameScriptingSystem();

            void onUpdate(AppTiming timing, entt::registry& reg) override;
            void onEvent(Event::Event& e) override;

            std::string getSysName() const override { return "Game Scripting System"; }

            void onAttach();
            void onDetach();

            void attachScript(entt::entity entity, const std::string& scriptPath);
            void attachScriptWithVars(entt::entity entity, const std::string& scriptPath,
                const std::vector<ScriptExternalVar>& vars);
            void onCollision(entt::entity entityA, entt::entity entityB);

            // public wrappers
            void setPathService(PAIN::Path::Path* fs);
            bool loadAllScriptsForEntityFromVDir(entt::entity entity, const std::string& alias,
                const std::string& relativeRoot, bool recursive = true,
                bool runWhenPaused = false);

        private:
            LuaManager luaManager_;
            std::optional<std::string> getKeyName(int keyCode) const;
        };
    }

}  // namespace PAIN








//#pragma once
//
//#include "Applications/AppSystem.h"
//#include "CoreSystems/Scripting/luaManager.h"
//#include "CoreSystems/Scripting/Bridge.h"
//#include <optional>
//
//namespace PAIN {
//
//    class GameScriptingSystem : public AppSystem {
//    public:
//        GameScriptingSystem() = default;
//        ~GameScriptingSystem() override = default;
//
//        // AppSystem lifecycle hooks
//        void onAttach() override;
//        void onDetach() override;
//        void onUpdate(AppTiming timing) override;
//        void onEvent(Event::Event& e) override;
//
//        // Entity scripting API
//        void attachScript(int entityId, const std::string& scriptPath);
//        void attachScriptWithVars(int entityId, const std::string& scriptPath,
//            const std::vector<ScriptExternalVar>& vars);
//
//        // Collision forwarding (call from physics system)
//        void onCollision(int entityA, int entityB);
//
//        // Access Lua manager
//        LuaManager& getLuaManager() { return luaManager_; }
//
//    private:
//        LuaManager luaManager_;
//        std::optional<Scripting::EngineWiring> engineWiring_;
//    };
//
//}  // namespace PAIN
