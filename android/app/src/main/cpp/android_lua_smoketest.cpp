#include <android/asset_manager.h>
//#include "PAINEngine/CoreSystems/Scripting/luaManager.h"
//#include "PAINEngine/CoreSystems/Scripting/EngineAPIAdapter.h"
#include "PAINEngine/CoreSystems/Scripting/Bridge.h"

extern AAssetManager* g_AssetMgr;

//static std::optional<PAIN::Scripting::EngineWiring> gWiring;
//static PAIN::LuaManager gLua;

//-----------------------------------------------
// Basic sol2 LuaState test (no LuaManager)
//-----------------------------------------------
static void RunLuaStateSmokeTest() {
    // PAIN::Scripting::LuaState L;
    // L.init(false);
    // L.doFile("game/scripts/test_2.lua");
}

//-----------------------------------------------
// Minimal LuaManager test 
//-----------------------------------------------
static void RunLuaManagerSmokeTest() {
    // gWiring = PAIN::Scripting::CreateMinimalEngineForAndroid();

    // //gLua.setPathService(gWiring->fs.get());
    // gLua.init(gWiring->api, /*shipping=*/true);

    // gLua.loadScriptForEntity(1, "game/scripts/test_2.lua", {}, true);
    // gLua.tick(0.016f);
}

//-----------------------------------------------
// Entry call from android_entry.cpp
//-----------------------------------------------
extern "C" void PAIN_RunLuaSmokeTests() {
    //RunLuaStateSmokeTest();
    RunLuaManagerSmokeTest();
}
