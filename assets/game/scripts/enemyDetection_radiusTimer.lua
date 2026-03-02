
-- radius/proximity-based (enemy detection) script WITH TIMER
 
-- Player Entity
local player = nil

-- Time that the player has been seen
local timeSeen = 0.0

-- Time required to be seen before caught
local timeRequired = 0.5

-- Whether player has been caught
local playerCaught = false

-- If the player is being detected (For detection UI)
local playerDetected = false
local minimapTagged = false

registerUpdate(function(dt)
    if (not minimapTagged) and addTag then
        addTag(entityId, "Enemy")
        addTag(entityId, "danger")
        addTag(entityId, "danger_radius")
        minimapTagged = true
    end

    local p = _G.PlayerEntity
    if not p then
        timeSeen = 0.0
        playerCaught = false
        return
    end
    player = p

    -- 1) If the game is already ended, do nothing.
    if PlayerState and PlayerState.isGameEnded and PlayerState.isGameEnded() then
        playerDetected = false
        timeSeen = 0.0
        playerCaught = false
        return
    end

    -- so player not detected when hiding and cooldown
    if PlayerState and PlayerState.canBeCaught and not PlayerState.canBeCaught() then
        playerDetected = false
        timeSeen = 0.0
        playerCaught = false
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

        -- Mark player as being detected
        playerDetected = true

        -- player in detection range
        -- log("[enemyDetection_radius] I see the player!")    

        if (not caught) and timeSeen >= timeRequired then
            playerCaught = true
            log("[EnemyDetection] Player playerDetected long enough - caught!")

            -- If player has been caught
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

        playerDetected = false

        -- Leaving range makes timer decay
        timeSeen = math.max(0.0, timeSeen - dt * 2.0)

        playerCaught = false
    end
end)
