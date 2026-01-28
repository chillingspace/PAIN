-- enemyDetectionLight.lua
-- attach script to light enemy

local player = nil
local enemy = entityId
local seeingPlayer = false

-- match inspector values
local LIGHT_OFFSET = { x = 0.0, y = 0.0, z = 0.0 }   -- same as Lighting.offset
local OUTER_ANGLE_DEG = 17.5                          -- spotlight outer angle
local MAX_DISTANCE = 9.0                              -- how far the light reaches (world units)

-- spotlight points straight down in world space for now
local LIGHT_DIR = { x = 0.0, y = -1.0, z = 0.0 }

local function normalize(x, y, z)
    local len = math.sqrt(x*x + y*y + z*z)
    if len <= 1e-6 then return 0.0, 0.0, 0.0, 0.0 end
    return x/len, y/len, z/len, len
end

registerUpdate(function(dt)
    -- cache player
    if not player then
        -- if already use _G.PlayerEntity, prefer that
        player = _G.PlayerEntity or findEntity("Player")
        if not player then return end
    end

    -- if you have player invincibility / respawn logic, respect it
    if PlayerState and PlayerState.canBeCaught and not PlayerState.canBeCaught() then
        -- if we were seeing before, cancel
        if seeingPlayer and DetectionUI and DetectionUI.cancel then
            DetectionUI.cancel(enemy)
        end
        seeingPlayer = false
        return
    end

    -- enemy/world position
    local ex, ey, ez = getPosition(enemy)

    -- light origin = enemy position + light offset
    local ox = ex + LIGHT_OFFSET.x
    local oy = ey + LIGHT_OFFSET.y
    local oz = ez + LIGHT_OFFSET.z

    -- player position
    local px, py, pz = getPosition(player)

    -- vector from light to player
    local vx = px - ox
    local vy = py - oy
    local vz = pz - oz

    -- early out on distance
    local vxN, vyN, vzN, dist = normalize(vx, vy, vz)
    if dist == 0.0 or dist > MAX_DISTANCE then
        return
    end

    -- angle between light direction and vector to player
    local lxN, lyN, lzN = normalize(LIGHT_DIR.x, LIGHT_DIR.y, LIGHT_DIR.z)
    local dot = vxN*lxN + vyN*lyN + vzN*lzN
    dot = math.max(-1.0, math.min(1.0, dot))  
    local angleDeg = math.deg(math.acos(dot))

    -- if angleDeg <= OUTER_ANGLE_DEG then
    --     log("[enemyLightTrigger] Player is in the enemy light cone!")

    --     if PlayerState and PlayerState.onCaught then
    --         PlayerState.onCaught(player)
    --     end
    -- end

    local inCone = (dist > 0.0 and dist <= MAX_DISTANCE and angleDeg <= OUTER_ANGLE_DEG)

    if inCone then
        if not seeingPlayer then
            seeingPlayer = true
            if DetectionUI and DetectionUI.begin then
                DetectionUI.begin(enemy)
            end
        end
    else
        if seeingPlayer then
            seeingPlayer = false
            if DetectionUI and DetectionUI.cancel then
                DetectionUI.cancel(enemy)
            end
        end
    end

end)