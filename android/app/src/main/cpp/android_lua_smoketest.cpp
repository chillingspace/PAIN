#include <android/asset_manager.h>

#include "PAINEngine/CoreSystems/Scripting/luaState.h"
#include "PAINEngine/CoreSystems/Scripting/luaManager.h"

extern AAssetManager* g_AssetMgr;

// forwars declare 
namespace PAIN {
namespace ECS  { struct Controller {}; }
namespace MetaData { struct Service {}; }
}

static LuaManager gLua;

//-----------------------------------------------
// Basic sol2 LuaState test (no LuaManager)
//-----------------------------------------------
static void RunLuaStateSmokeTest() {
    PAIN::Scripting::LuaState L;
    L.init(false);
    L.doFile("game/scripts/test.lua");
}

//-----------------------------------------------
// Minimal LuaManager test 
//-----------------------------------------------
static void RunLuaManagerSmokeTest() {
    gLua.init(nullptr, /*shipping=*/true);
    gLua.loadScriptForEntity(1, "game/scripts/test.lua", {}, true);
    gLua.tick(0.016);
}

//-----------------------------------------------
// Entry call from android_entry.cpp
//-----------------------------------------------
extern "C" void PAIN_RunLuaSmokeTests() {
    //RunLuaStateSmokeTest();
    RunLuaManagerSmokeTest();
}
