-- enemyDetectionLight.lua
-- attach script to light enemy

local player = nil
local enemy = entityId
local seeingPlayer = false
local minimapTagged = false

-- Approximate player height so we test feet, torso, and head — not just the pivot.
-- The pivot is typically at the feet; adjust if your player origin is at centre.
local PLAYER_HEIGHT     = 1.8   -- world units (tune to match the player model)
local LOST_SIGHT_GRACE  = 0.12  -- seconds before losing detection (matches ground enemy)

local loseSightTimer = 0.0

-- light colors
local NORMAL_COLOR = { r = 0.5, g = 0.5, b = 0.5 }
local ALERT_COLOR  = { r = 1.0, g = 0.1, b = 0.1 }

_G.EnemyFrozen = _G.EnemyFrozen or {}

-- Returns true if the world point (px,py,pz) lies inside the cone defined by
-- origin (ox,oy,oz), unit direction (ldx,ldy,ldz), and half-angle tangent.
local function pointInCone(px, py, pz, ox, oy, oz, ldx, ldy, ldz, halfTan)
    local vx = px - ox
    local vy = py - oy
    local vz = pz - oz
    -- depth along the cone axis (positive = in front of the cone)
    local depth = vx*ldx + vy*ldy + vz*ldz
    if depth <= 0.0 then return false end
    -- squared off-axis distance (Pythagoras: |v|^2 - depth^2)
    local offSq = vx*vx + vy*vy + vz*vz - depth*depth
    if offSq < 0.0 then offSq = 0.0 end
    -- inside cone: offAxis / depth <= tan(halfAngle)  =>  offSq <= depth^2 * halfTan^2
    return offSq <= depth * depth * halfTan * halfTan
end

setLightIntensity(enemy, NORMAL_COLOR.r, NORMAL_COLOR.g, NORMAL_COLOR.b)

registerUpdate(function(dt)
    if (not minimapTagged) and addTag then
        addTag(enemy, "Enemy")
        addTag(enemy, "danger")
        addTag(enemy, "light_cone")
        minimapTagged = true
    end

    -- cache player
    if not player then
        player = _G.PlayerEntity or findEntity("Player")
        if not player then return end
    end

    -- respect invincibility / respawn / hidden states
    if PlayerState and PlayerState.canBeCaught and not PlayerState.canBeCaught() then
        if seeingPlayer and DetectionUI and DetectionUI.cancel then
            DetectionUI.cancel(enemy)
        end
        seeingPlayer = false
        loseSightTimer = 0.0
        setLightIntensity(enemy, NORMAL_COLOR.r, NORMAL_COLOR.g, NORMAL_COLOR.b)
        if not (DetectionUI and DetectionUI.active) then
            _G.EnemyFrozen[enemy] = false
        end
        return
    end

    -- Read actual light properties from the engine so they always match the editor values.
    -- getWorldPosition uses WorldTransform (full hierarchy) — safe for parented entities.
    -- getLightOffset / getLightDirection / getLightOuterAngle read the Lighting component.
    local ex, ey, ez = getWorldPosition(enemy)
    local lox, loy, loz = getLightOffset(enemy)
    local ox = ex + lox
    local oy = ey + loy
    local oz = ez + loz

    local ldx, ldy, ldz = getLightDirection(enemy)
    local outerAngleDeg  = getLightOuterAngle(enemy)
    local halfTan        = math.tan(math.rad(outerAngleDeg))

    -- Player world position (pivot, typically at feet).
    local px, py, pz = getWorldPosition(player)

    -- Test three vertical sample points so the full player body is considered:
    --   feet (pivot), torso (half height), head (full height).
    -- Detection succeeds if any sample point is inside the cone.
    local inCone = pointInCone(px, py,                    pz, ox, oy, oz, ldx, ldy, ldz, halfTan)
               or  pointInCone(px, py + PLAYER_HEIGHT*0.5, pz, ox, oy, oz, ldx, ldy, ldz, halfTan)
               or  pointInCone(px, py + PLAYER_HEIGHT,     pz, ox, oy, oz, ldx, ldy, ldz, halfTan)

    -- If inside the cone, also verify no geometry blocks line of sight
    if inCone then
        inCone = hasLineOfSight(ox, oy, oz, px, py, pz)
              or hasLineOfSight(ox, oy, oz, px, py + PLAYER_HEIGHT * 0.5, pz)
              or hasLineOfSight(ox, oy, oz, px, py + PLAYER_HEIGHT, pz)
    end

    if inCone then
        loseSightTimer = 0.0

        if not seeingPlayer then
            seeingPlayer = true
            _G.EnemyFrozen[enemy] = true
            if DetectionUI and DetectionUI.begin then
                DetectionUI.begin(enemy)
            end
            setLightIntensity(enemy, ALERT_COLOR.r, ALERT_COLOR.g, ALERT_COLOR.b)
        elseif DetectionUI and not DetectionUI.active then
            _G.EnemyFrozen[enemy] = true
            if DetectionUI.begin then DetectionUI.begin(enemy) end
        end
    else
        if seeingPlayer then
            -- Grace period: require the player to be outside the cone for
            -- LOST_SIGHT_GRACE seconds before actually cancelling detection.
            -- This prevents flicker from jitter / animation bobbing / cone boundaries.
            loseSightTimer = loseSightTimer + dt
            if loseSightTimer < LOST_SIGHT_GRACE then
                return  -- still within grace — keep current detection state
            end

            -- Grace elapsed — commit to losing the player
            seeingPlayer = false
            loseSightTimer = 0.0
            if DetectionUI and DetectionUI.cancel then
                DetectionUI.cancel(enemy)
            end
            setLightIntensity(enemy, NORMAL_COLOR.r, NORMAL_COLOR.g, NORMAL_COLOR.b)
        end

        -- only unfreeze once the ui bar is fully drained
        if not (DetectionUI and DetectionUI.active) then
            _G.EnemyFrozen[enemy] = false
        end
    end

end)
