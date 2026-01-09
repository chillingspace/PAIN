
log("[LUA] test_2.lua running")

log("[LUA] engineAvailable:", engineAvailable and engineAvailable() or "<nil>")

local id = createEntity("Default", "TestEntity")
log("[LUA] Created entity ID:", id)

setEntityName(id, "MyLuaEntity")
log("[LUA] Name:", getEntityName(id))

setPosition(id, 10, 20, 30)
local x,y,z = getPosition(id); log("[LUA] Position:", x,y,z)

setScale(id, 1.3, 2.5, 3.1)
local sx,sy,sz = getScale(id); log("[LUA] Scale:", sx,sy,sz)

local meshId = getMeshId(id);      log("[LUA] Mesh initial:", meshId)
setMeshId(id, 1234)
log("[LUA] Mesh after set:", getMeshId(id))

log("[LUA] HasLight (before):", hasLight(id))
addLight(id)
log("[LUA] HasLight (after):", hasLight(id))
