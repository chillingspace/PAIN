#pragma once
//#include <sol/sol.hpp>
#include "sol_sanitized.h"
#include <unordered_map>
#include <list>
#include <optional>
#include <queue>
#include "PAINEngine/CoreSystems/Events/Event.h"
#include "PAINEngine/CoreSystems/Scene/sCameraController.h"
#include "PAINEngine/ECS/System/ISystem.h"
#include <entt/entity/entity.hpp>
#include "PAINEngine/CoreSystems/Path/Path.h"

struct ScriptExternalVar { std::string id; std::variant<std::string, double, bool> val; };

namespace PAIN {
    //namespace Path { class Path; } 
    //}

    class IEngineAPI;

    class LuaManager {
    public:
        struct LuaFunction { entt::entity entityId; sol::protected_function fn; bool runWhenPaused = false; };
        struct CollisionLuaFunction { entt::entity currentEntityId; entt::entity collidedEntityId; sol::protected_function fn; bool hasCollided = false; bool runWhenPaused = false; };
        struct MouseInOutLuaFunction {
            enum class State { MouseIn, MouseOut };
            int entityId; LuaFunction mouseIn; LuaFunction mouseOut; bool runWhenPaused = false; State state = State::MouseOut;
        };
        struct TimeoutCB { sol::protected_function fn; float remaining; };
        struct CollisionInterest { entt::entity entityInterested; entt::entity entityToCheck; };

    public:
        void init(std::shared_ptr<IEngineAPI> api, bool shipping);
        bool loadScriptForEntity(entt::entity entityId, const std::string& filePath,
            const std::vector<ScriptExternalVar>& vars = {}, bool runWhenPaused = false);

        void tick(double dt);
        void setPrefabInstantiator(std::function<entt::entity(const std::string& prefab,
            const std::string& layer,
            const std::string& name)> fn);

        // engine -> lua events
        void onKeyDown(const std::string& name);
        void onKeyUp(const std::string& name);
        void onClick();
        void onMouseInOut();
        void onCollision(entt::entity a, entt::entity b);
        void onPauseChanged(bool paused);
        const std::vector<CollisionInterest>& getCollisionInterests() const { return collisionInterests_; }

        // misc
        void callGlobal(const std::string& name);
        void callGlobal(const std::string& name, const std::string& arg1, entt::entity arg2, const std::string& arg3);
        void callGlobalWithVec2(const std::string& name, float x, float y);
        void queueOp(std::function<void(void)> op) { delayedOps_.push_back(std::move(op)); }
        void setPendingSceneChange(std::function<void(void)> op) { pendingSceneChange_ = std::move(op); }

        void setPathService(PAIN::Path::Path* fs) { fs_ = fs; }
        bool loadAllScriptsForEntityFromVDir(
            entt::entity entityId,
            const std::string& alias,
            const std::string& relativeRoot,
            bool recursive = true,
            bool runWhenPaused = false);

        void Input_OnEvent(PAIN::Event::Event& e);
        void Input_EndFrame();
        void onDetach();
        void resetForSceneReload();

    public:
        sol::state& state() { return lua_; } // so AI can call into Lua directly
        const sol::state& state() const { return lua_; }

    private:
        // bindings
        void openLibs(bool shipping);
        void bindUsertypes();
        void bindRegistration();
        void bindEngineAPI();

        bool runFileIntoEnv(const std::string& path, entt::entity entityId, const std::vector<ScriptExternalVar>& vars, bool runWhenPaused);

        static entt::entity toEntity(int id);

    private:
        struct TimeoutNode {
            double wake;
            sol::protected_function fn;
            bool operator<(const TimeoutNode& other) const noexcept { return wake > other.wake; } // priorityqueue is max-heap by default, invert comparator for min-heap
        };
        std::priority_queue<TimeoutNode> timeoutHeap_;
        bool sceneChangeQueued_ = false;

        sol::state lua_;
        std::shared_ptr<IEngineAPI> api_;
        bool shipping_{ false };
        bool gamePaused_{ false };

        // registries 
        std::vector<LuaFunction> updates_;
        std::unordered_map<std::string, std::vector<LuaFunction>> keyDown_, keyUp_;
        std::vector<LuaFunction> onClick_;
        std::vector<MouseInOutLuaFunction> mouseInOut_;
        std::unordered_map<entt::entity, std::vector<CollisionLuaFunction>> onCollision_;
        std::vector<CollisionInterest> collisionInterests_;
        std::vector<LuaFunction> pauseHandlers_;

        std::list<LuaFunction> inputQueue_;
        std::list<CollisionLuaFunction> collisionQueue_;
        std::list<TimeoutCB> timeouts_;
        std::list<std::function<void(void)>> delayedOps_;
        std::optional<std::function<void(void)>> pendingSceneChange_;

        entt::entity currentEntity_{ entt::null };
        bool currentRunWhenPaused_{ false };
        PAIN::Path::Path* fs_{ nullptr };
        std::function<entt::entity(const std::string&, const std::string&, const std::string&)> instantiatePrefab_;

    };

} // PAIN namespace
