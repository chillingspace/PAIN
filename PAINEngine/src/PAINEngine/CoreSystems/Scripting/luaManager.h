#pragma once
//#include <sol/sol.hpp>
#include "sol_sanitized.h"
#include <unordered_map>
#include <list>
#include <optional>

struct ScriptExternalVar { std::string id; std::variant<std::string, double, bool> val; };

class IEngineAPI;

class LuaManager {
public:
    struct LuaFunction { int entityId; sol::protected_function fn; bool runWhenPaused = false; };
    struct CollisionLuaFunction { int currentEntityId; int collidedEntityId; sol::protected_function fn; bool hasCollided = false; };
    struct MouseInOutLuaFunction {
        enum class State { MouseIn, MouseOut };
        int entityId; LuaFunction mouseIn; LuaFunction mouseOut; bool runWhenPaused = false; State state = State::MouseOut;
    };
    struct TimeoutCB { sol::protected_function fn; float remaining; };
    struct CollisionInterest { int entityInterested; int entityToCheck; };

public:
    void init(IEngineAPI* api, bool shipping);
    bool loadScriptForEntity(int entityId, const std::string& filePath,
        const std::vector<ScriptExternalVar>& vars = {}, bool runWhenPaused = false);
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

private:
    // bindings
    void openLibs(bool shipping);
    void bindUsertypes();
    void bindRegistration();
    void bindEngineAPI();

    bool runFileIntoEnv(const std::string& path, int entityId, const std::vector<ScriptExternalVar>& vars, bool runWhenPaused);

private:
    sol::state lua_;
    IEngineAPI* api_{ nullptr };
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
};
