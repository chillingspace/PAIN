
-- radius/proximity-based (enemy detection) script 

local player = nil

registerUpdate(function(dt)
    if not player then
        player = findEntity("Player")
        if not player then 
            return 
        end

        -- ensure PlayerState knows the start/checkpoint
        if PlayerState and PlayerState.init then
            PlayerState.init(player)
        end
    end

    local ex, ey, ez = getPosition(entityId)
    local px, py, pz = getPosition(player)
    local dx, dy, dz = px - ex, py - ey, pz - ez
    local dist2 = dx*dx + dy*dy + dz*dz

    if dist2 < 3*3 then
        -- player in detection range
        log("[enemyDetection_radius] I see the player!")
        
        -- so player not detected when hiding and cooldown
        if PlayerState and PlayerState.canBeCaught and not PlayerState.canBeCaught() then
            return
        end

        if PlayerState and PlayerState.onCaught then
            PlayerState.onCaught(player)
        end
    end
end)
