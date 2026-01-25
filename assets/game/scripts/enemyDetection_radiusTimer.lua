
-- radius/proximity-based (enemy detection) script WITH TIMER
 

local player = nil
local timeSeen = 0.0
local timeRequired = 3.0
local caught = false

registerUpdate(function(dt)
    local p = _G.PlayerEntity
    if not p then
        timeSeen = 0.0
        caught = false
        return
    end
    player = p

    -- 1) If the game is already ended, do nothing.
    if PlayerState and PlayerState.isGameEnded and PlayerState.isGameEnded() then
        timeSeen = 0.0
        caught = false
        return
    end

    -- so player not detected when hiding and cooldown
    if PlayerState and PlayerState.canBeCaught and not PlayerState.canBeCaught() then
        timeSeen = 0.0
        caught = false
        return
    end

    -- log("[EnemyDetection] player cached:", tostring(player), "found:", tostring(_G.PlayerEntity))

    local ex, ey, ez = getPosition(entityId)
    local px, py, pz = getPosition(player)
    local dx, dy, dz = px - ex, py - ey, pz - ez
    local dist2 = dx*dx + dy*dy + dz*dz

    local BASE_DETECTION_RADIUS = 1.0 -- 0.35

    local sx, sy, sz = getScale(entityId)
    local radius = BASE_DETECTION_RADIUS * math.max(sx, sz)
    local inRange = dist2 < radius*radius
    
    -- log(string.format("[EnemyDetection] seen %.2f/%.2f", timeSeen, timeRequired))  

    if inRange then
        -- Increase time seen
        timeSeen = timeSeen + dt

        -- player in detection range
        -- log("[enemyDetection_radius] I see the player!")    

        if (not caught) and timeSeen >= timeRequired then
            caught = true
            log("[EnemyDetection] Player detected long enough - caught!")

            if PlayerState and PlayerState.onCaught then
                -- Reset time seen
                timeSeen = 0.0

                -- Catch player
                PlayerState.onCaught(player)
            end
        end
    -- Not in range
    else
        -- Leaving range resets the timer
        -- timeSeen = 0.0

        -- Leaving range makes timer decay
        timeSeen = math.max(0.0, timeSeen - dt * 2.0)
        caught = false
    end
end)
