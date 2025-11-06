#pragma once
//#include <sol/sol.hpp>
#include "sol_sanitized.h"
#include <unordered_map>
#include <list>
#include <optional>
#include <queue>
#include "PAINEngine/CoreSystems/Events/Event.h"

struct ScriptExternalVar { std::string id; std::variant<std::string, double, bool> val; };

class IEngineAPI;
namespace PAIN { namespace Path { class Path; } }

class LuaManager {
public:
    struct LuaFunction { int entityId; sol::protected_function fn; bool runWhenPaused = false; };
    struct CollisionLuaFunction { int currentEntityId; int collidedEntityId; sol::protected_function fn; bool hasCollided = false; bool runWhenPaused = false; };
    struct MouseInOutLuaFunction {
        enum class State { MouseIn, MouseOut };
        int entityId; LuaFunction mouseIn; LuaFunction mouseOut; bool runWhenPaused = false; State state = State::MouseOut;
    };
    struct TimeoutCB { sol::protected_function fn; float remaining; };
    struct CollisionInterest { int entityInterested; int entityToCheck; };

public:
    void init(std::shared_ptr<IEngineAPI> api, bool shipping);
    bool loadScriptForEntity(int entityId, const std::string& filePath,
        const std::vector<ScriptExternalVar>& vars = {}, bool runWhenPaused = false);
    
    // execution order each frame:
    // 1. Timeouts that are due
    // 2. Input callbacks (key/mouse/click)
    // 3. Collision callbacks  
    // 4. Update callbacks
    // 5. Delayed operations
    // 6. Pending scene change
    void tick(double dt);

    // engine -> lua events
    void onKeyDown(const std::string& name);
    void onKeyUp(const std::string& name);
    void onClick();
    void onMouseInOut();
    void onCollision(int a, int b);
    void onPauseChanged(bool paused);
    const std::vector<CollisionInterest>& getCollisionInterests() const { return collisionInterests_; }

    // misc
    void callGlobal(const std::string& name);
    void queueOp(std::function<void(void)> op) { delayedOps_.push_back(std::move(op)); }
    void setPendingSceneChange(std::function<void(void)> op) { pendingSceneChange_ = std::move(op); }

    void setPathService(PAIN::Path::Path* fs) { fs_ = fs; }
    bool loadAllScriptsForEntityFromVDir(
        int entityId,
        const std::string& alias,
        const std::string& relativeRoot,
        bool recursive = true,
        bool runWhenPaused = false);

    void Input_OnEvent(PAIN::Event::Event& e);
    void Input_EndFrame();
    void onDetach();

private:
    // bindings
    void openLibs(bool shipping);
    void bindUsertypes();
    void bindRegistration();
    void bindEngineAPI();

    bool runFileIntoEnv(const std::string& path, int entityId, const std::vector<ScriptExternalVar>& vars, bool runWhenPaused);

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
    std::unordered_map<int, std::vector<CollisionLuaFunction>> onCollision_;
    std::vector<CollisionInterest> collisionInterests_;
    std::vector<LuaFunction> pauseHandlers_;

    std::list<LuaFunction> inputQueue_;
    std::list<CollisionLuaFunction> collisionQueue_;
    std::list<TimeoutCB> timeouts_;
    std::list<std::function<void(void)>> delayedOps_;
    std::optional<std::function<void(void)>> pendingSceneChange_;

    int currentEntity_{ -1 };
    bool currentRunWhenPaused_{ false };
    PAIN::Path::Path* fs_{ nullptr };
};
