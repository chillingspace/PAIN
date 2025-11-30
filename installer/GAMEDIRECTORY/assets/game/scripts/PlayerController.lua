
-- -- Test_01 entity creation/deletion
-- local createdEntity = nil
-- registerUpdate(function(dt)
--     if wasKeyPressed(257) then  -- Enter
--         log("ENTER pressed!")
--         if createdEntity == nil then
--             createdEntity = createEntity("default", "TestEntity")
--             log("created entity:", createdEntity)
--         else
--             deleteEntity(createdEntity)
--             log("deleted entity:", createdEntity)
--             createdEntity = nil
--         end
--     end
-- end)


-- -- Test_02 entity creation/deletion with registerKeyDown 
-- local createdEntity = nil
-- registerKeyDown("ENTER", function()
--     if createdEntity == nil then
--         createdEntity = createEntity("default", "TestEntity")
--         log("created entity:", createdEntity)
--     else
--         deleteEntity(createdEntity)
--         log("deleted entity:", createdEntity)
--         createdEntity = nil
--     end
-- end)


-- -- Test_03 finding entities 
-- registerUpdate(function(dt)
--     local found = findEntity("ogre_left") 
--     if found then
--         log("found entity:", found)
--     end
-- end)


-- -- Test_04 entity naming
-- local counter = 0
-- registerUpdate(function(dt)
--     counter = counter + 1
--     setEntityName(entityId, "DynamicName_" .. counter % 10)
--     local name = getEntityName(entityId)
--     log("entity name:", name)
-- end)


-- -- Test_05 tag system 
-- registerUpdate(function(dt)
--     addTag(entityId, "player_controlled")
--     addTag(entityId, "interactive")
    
--     if hasTag(entityId, "player_controlled") then
--         log("HAS player_controlled tag")
--     end
    
--     if hasTag(entityId, "interactive") then
--         log("HAS interactive tag")
--     end

--     removeTag(entityId, "interactive")

--     if hasTag(entityId, "interactive") then
--         log("HAS interactive tag")
--     else 
--         log("NOT have interactive tag")
--     end
-- end)


-- -- Test_06 group assignment
-- registerUpdate(function(dt)
--     assignGroup(entityId, "enemies")
--     local group = getGroup(entityId)
--     log("assigned to group:", group)
-- end)


-- -- Test_07 lighting API 
-- local t = 0
-- local intensity = 0
-- registerUpdate(function(dt)
--     t = t + dt
    
--     if not hasLight(entityId) then
--         addLight(entityId)
--         log("Added light to entity:", entityId)

--         setLightType(entityId, 0)  -- Point light
--         log("Set light type to 0 (point light)")
--     end
    
--     -- Pulsing light intensity
--     intensity = 0.5 + 0.5 * math.sin(2 * t)
--     setLightIntensity(entityId, intensity, intensity, intensity)

--     setLightPosition(entityId, 0, 5, 0)
--     log("Light update - t:", math.floor(t*100)/100, 
--         "intensity:", math.floor(intensity*100)/100)
    
--     -- Log once per second for readability
--     if math.floor(t) % 1 == 0 and math.floor(t*10) % 10 == 0 then
--         log("Lighting working! Entity:", entityId)
--     end
-- end)


-- -- Test_08 KeyDown
-- registerKeyDown("A", function()
--     log("A key pressed!")
-- end)

-- registerKeyDown("SPACE", function()
--     log("SPACE pressed!")
-- end)

-- registerKeyDown("ENTER", function()
--     log("ENTER pressed!")
-- end)


-- -- Test_09 create entity with mesh assignment 
-- local testEntity = nil
-- local meshAssigned = false

-- registerKeyDown("C", function()
--     if not testEntity then
--         -- Create a new entity
--         testEntity = createEntity("default", "MeshTestEntity")
--         log("created test entity:", testEntity)
        
--         -- Assign a mesh ID (use any valid ID from your system)
--         -- For testing, we'll try a simple ID like 0, 1, etc.
--         setMeshId(testEntity, 1)  -- Assign mesh ID 1
--         log("assigned mesh ID 1 to entity:", testEntity)
--         meshAssigned = true
--     end
-- end)

-- registerKeyDown("D", function()
--     if testEntity then
--         -- Delete the test entity
--         deleteEntity(testEntity)
--         log("deleted test entity:", testEntity)
--         testEntity = nil
--         meshAssigned = false
--     end
-- end)

-- registerKeyDown("E", function()
--     if meshAssigned and testEntity then
--         -- Retrieve and display the mesh
--         local meshId = getMeshId(testEntity)
--         if meshId then
--             log("current mesh on entity", testEntity, ":", meshId)
--         else
--             log("no mesh assigned to entity", testEntity)
--         end
--     else
--         log("no test entity created (press C)")
--     end
-- end)

-- registerUpdate(function(dt)
--     -- Show status every 2 seconds
--     if meshAssigned and testEntity and math.floor(math.random() * 120) == 0 then
--         local meshId = getMeshId(testEntity)
--         log("mesh persistence check - entity:", testEntity, "mesh:", meshId)
--     end
-- end)


-- -- Test_10 transform position
-- registerUpdate(function(dt)
--     local x,y,z = getPosition(entityId)
--     setPosition(entityId, x + 1.0 * dt, y, z)
--     log("moved ->", x + 1.0*dt, y, z)
-- end)


-- -- Test_11 transform scale
-- local t = 0
-- registerUpdate(function(dt)
--     t = t + dt
--     local s = 1.0 + 0.25 * math.sin(4*t)
--     setScale(entityId, s, s, s)
--     local sx, sy, sz = getScale(entityId)
--     log("scale ->", sx, sy, sz)
-- end)



-- -- Test_12 KeyUp KeyDown
-- registerKeyDown("SPACE", function()
--     log("SPACE down")
-- end)

-- registerKeyUp("SPACE", function()
--     log("SPACE up")
-- end)


-- -- Test_13 mouse 
-- registerUpdate(function(dt)
--     if isMouseDown(0) then
--         local p = mousePos()
--         log("[LUA] mouse:", p.x, p.y)
--     end
-- end)


-- -- Test_14 mouse scroll
-- registerUpdate(function(dt)
--     local s = mouseScroll()
--     if s.x ~= 0 or s.y ~= 0 then
--         log("mouse scroll:", s.x, s.y)
--     end
-- end)


-- -- Test_15 registerOnClick
-- registerOnClick(function()
--     log("[LUA] 09 clicked entity:", entityId)
-- end)


-- -- Test_16 schedule timeout
-- log("[LUA] scheduling timeout - 15 seconds")

-- setTimeout(function()
--     log("[LUA] timeout fired - 15 seconds complete")
-- end, 15.0)


-- -- Test_17 pause -> not done
-- registerPauseHandler(function()
--     log("[LUA] 11 pause toggled. paused=", isGamePaused())
-- end)


-- -- Test_18 get image
-- local g = getImageID("game/textures/Heart.png")
-- if g == "" then
--   log("not found")
-- else
--   log("guid = ", g)
-- end


-- Test_19 play sound
registerUpdate(function(dt)
    if wasKeyPressed(257) then -- enter
        log("[AudioTest] ENTER pressed, playing sound")
        base.audioPlay(entityId)
    end
end)



------------------------------------------------
-- local ogre_right = findEntity("ogre_right")
-- local ogre_left  = findEntity("ogre_left")

-- print("[Lua] ogre_right =", ogre_right)
-- print("[Lua] ogre_left  =", ogre_left)

-- -- setPosition(ogre_right, 0, 0, 0)
-- -- setPosition(ogre_left, 0, 0, 0)

-- registerOnCollision(function(self, other) 
--     print("filtered", self, other) end, ogre_left)
-- -- registerOnCollision(function(self, other) print("any", self, other) end, nil)


-- -- -- engine exposes getEntityByName("Player")
-- -- local player = getEntityByName and getEntityByName("Player") or 0

-- -- registerOnCollision(function(self, other)
-- --   print("[Lua] HIT Player only:", self, other)
-- -- end, player)
------------------------------------------------