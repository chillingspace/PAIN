
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
    local radius = 0.35

    if dist2 < radius*radius then
        -- 1) If the game is already ended, do nothing.
        if PlayerState and PlayerState.isGameEnded and PlayerState.isGameEnded() then
            return
        end

        -- so player not detected when hiding and cooldown
        if PlayerState and PlayerState.canBeCaught and not PlayerState.canBeCaught() then
            return
        end

        -- player in detection range
        log("[enemyDetection_radius] I see the player!")      

        if PlayerState and PlayerState.onCaught then
            PlayerState.onCaught(player)
        end
    end
end)
