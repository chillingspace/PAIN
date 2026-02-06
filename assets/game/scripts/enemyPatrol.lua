
-- enemyPatrol.lua 
-- moves from one end to another end

local speed       = 2.5   
local waitTime    = 1.0 -- pause at each end
local startTag    = "enemy_patrol_start"
local endTag      = "enemy_patrol_end"
local searchRadius = 15.0 -- how far to look for waypoints
local patrolInset  = 1.5 -- how far in each endpoint so it doesnt slam into walls etc

local pointA = nil
local pointB = nil
local goingToB = true
local waitTimer = 0.0
local waitYaw = nil

-- SFX for enemy movement (spatial 3D audio)
local SFX_ENEMY_WALK = "game/audio/sfx/enemy/Ball enemy walking.wav"
local walkChannel = -1  -- Track channel for position updates and stopping

local function findNearestByTag(originX, originY, originZ, tag, maxDistSq)
    local candidates = getEntitiesByTag(tag)
    if not candidates or #candidates == 0 then
        return nil
    end

    local best = nil
    local bestDistSq = maxDistSq

    for _, e in ipairs(candidates) do
        local x, y, z = getPosition(e)
        local dx = x - originX
        local dy = y - originY
        local dz = z - originZ
        local distSq = dx*dx + dy*dy + dz*dz

        if distSq <= bestDistSq then
            bestDistSq = distSq
            best = { entity = e, x = x, y = y, z = z }
        end
    end

    return best
end

registerUpdate(function(dt)
    local id = entityId

    -- freeze while detecting
    if _G.EnemyFrozen and _G.EnemyFrozen[id] then
        -- Stop walking sound when frozen
        if walkChannel >= 0 then
            audioStopChannel(walkChannel)
            walkChannel = -1
        end
        return
    end

    if not pointA or not pointB then
        local ex, ey, ez = getPosition(id)

        -- 1. try to find a nearby "start" waypoint
        local startWp = findNearestByTag(ex, ey, ez, startTag, searchRadius * searchRadius)

        if startWp then
            pointA = { x = startWp.x, y = startWp.y, z = startWp.z }
        else
            pointA = { x = ex, y = ey, z = ez } -- fallback use enemys own position as start
        end

        -- 2. try to find a nearby "end" waypoint
        local endWp = findNearestByTag(ex, ey, ez, endTag, searchRadius * searchRadius)

        if endWp then
            pointB = { x = endWp.x, y = endWp.y, z = endWp.z }
        else
            pointB = { x = pointA.x + 5.0, y = pointA.y, z = pointA.z } -- fallback: just make up a simple offset forward
        end

        do -- inset logic
            local vx = pointB.x - pointA.x
            local vz = pointB.z - pointA.z
            local lenSq = vx*vx + vz*vz
            
            if lenSq > 0.0001 then
                local len = math.sqrt(lenSq)
                
                -- Only inset if segment is long enough (at least 2x the inset distance)
                local minPatrolDist = patrolInset * 3.0  -- Changed from 2.0 to 3.0 for safety
                
                if len > minPatrolDist then
                    local inset = math.min(patrolInset, len * 0.4)  -- Max 40% inset, not 50%
                    local ux = vx / len
                    local uz = vz / len
                    
                    -- move A forward along the line, B backward along the line
                    pointA.x = pointA.x + ux * inset
                    pointA.z = pointA.z + uz * inset
                    pointB.x = pointB.x - ux * inset
                    pointB.z = pointB.z - uz * inset
                end
            end
        end
    end

    -- wait
    if waitTimer > 0.0 then
        waitTimer = waitTimer - dt
        if waitTimer <= 0.0 then
            waitTimer = 0.0
            goingToB = not goingToB
            waitYaw = nil   
        end

        -- stop movement sound while waiting
        if walkChannel >= 0 then
            audioStopChannel(walkChannel)
            walkChannel = -1
        end

        -- keep the stored facing during wait
        if waitYaw and setRotation then
            setRotation(id, 0.0, waitYaw, 0.0)
        end

        return
    end

    local target = goingToB and pointB or pointA

    local x, y, z = getPosition(id)
    local dx = target.x - x
    local dy = target.y - y
    local dz = target.z - z

    local distSq = dx*dx + dy*dy + dz*dz
    local closeEnough = 0.2

        if distSq < closeEnough * closeEnough then
        waitTimer = waitTime

        local nextTarget = goingToB and pointA or pointB

        local nx = nextTarget.x - x
        local nz = nextTarget.z - z
        local ndSq = nx*nx + nz*nz

        if ndSq > 0.0001 then
            local nd = math.sqrt(ndSq)
            nx, nz = nx / nd, nz / nd

            waitYaw = math.atan(nx, nz)

            if setRotation then
                setRotation(id, 0.0, waitYaw, 0.0)
            end
        else
            waitYaw = nil
        end

        -- stop movement sound while waiting
        if walkChannel >= 0 then
            audioStopChannel(walkChannel)
            walkChannel = -1
        end

        return
    end



    local dist = math.sqrt(distSq)
    dx, dy, dz = dx / dist, dy / dist, dz / dist

    x = x + dx * speed * dt
    y = y + dy * speed * dt
    z = z + dz * speed * dt
    setPosition(id, x, y, z)

    if setRotation then
        local yaw = math.atan(dx, dz)
        setRotation(id, 0.0, yaw, 0.0)
    end

    -- Play/update movement sound while walking (spatial 3D)
    if walkChannel < 0 then
        -- Start new looping walk sound at enemy position
        walkChannel = audioPlaySFXAt(SFX_ENEMY_WALK, x, y, z, -6.0, true)
    else
        -- Update position of existing walk sound
        audioSetChannelPosition(walkChannel, x, y, z)
    end
end)