-- enemyCorridorChase.lua
-- Corridor enemy that:
--   waits idle until the player enters a trigger radius
--   fires a camera pan cinematic showing the enemy "waking up"
--   freezes briefly (ALERT_PAUSE), then charges the player (CHASE)
--   if player is caught: respawn at fixed spot, enemy resets to home
--   if player escapes (MAX_CHASE_DISTANCE): enemy walks home (RETURN)
--   trigger re-arms after every reset so the sequence replays on re-entry
--   crossfades global BGM to combat on ALERT_PAUSE→CHASE, back on IDLE

do
    local enemy = entityId
    local player = nil
    local minimapTagged = false
    local initialized = false

    -- =========================================================
    -- CONFIG
    -- =========================================================

    -- Trigger
    local TRIGGER_RADIUS        = 7.0   -- proximity to fire camera pan (units)

    -- Camera pan (uses _G.StartCameraPan from thirdPersonCamera.lua)
    local CAM_WAYPOINTS         = { "corridor_cam_waypoint_1" }
    local CAM_PAN_DURATION      = 2.0   -- seconds to travel between each waypoint
    local CAM_HOLD_DURATION     = 1.0   -- seconds to hold at each waypoint
    local CAM_RETURN_DURATION   = 1.0   -- seconds to ease back to player cam

    -- Timing
    local ALERT_PAUSE_TIME      = 0.5   -- freeze before charging (seconds)

    -- Movement
    local CHASE_SPEED           = 2.5   -- units/sec while chasing
    local RETURN_SPEED          = 2.0   -- units/sec walking back to home
    local MAX_CHASE_DISTANCE    = 35.0  -- give up if this far from home (units)
    local ARRIVE_EPSILON        = 0.25  -- close enough to home to stop
    -- Respawn
    local RESPAWN_ENTITY_NAME   = "corridor_enemy_respawn"

    -- Animation
    local ANIM_MOVE             = "ArmatureAction"

    -- =========================================================
    -- STATE CONSTANTS
    -- =========================================================

    local STATE_IDLE        = "IDLE"
    local STATE_CAM_PAN     = "CAM_PAN_WAIT"
    local STATE_ALERT_PAUSE = "ALERT_PAUSE"
    local STATE_CHASE       = "CHASE"
    local STATE_RETURN      = "RETURN"

    -- =========================================================
    -- STATE VARIABLES
    -- =========================================================

    local state       = STATE_IDLE
    local stateTimer  = 0.0
    local triggered   = false        -- proximity trigger armed/disarmed

    -- Home position (recorded at init from enemy's starting transform)
    local home        = { x = 0.0, y = 0.0, z = 0.0 }
    local baseRotX, baseYaw, baseRotZ = 0.0, 0.0, 0.0
    local currentYaw  = 0.0

    -- Cached respawn entity world position
    -- _G.PlayerState.checkpointPos expects a plain Lua table { x, y, z }
    local respawnPos  = nil

    -- =========================================================
    -- HELPERS
    -- =========================================================

    local function normalize(x, y, z)
        local len = math.sqrt(x*x + y*y + z*z)
        if len <= 1e-6 then return 0.0, 0.0, 0.0, 0.0 end
        return x/len, y/len, z/len, len
    end

    local function dist3(ax, ay, az, bx, by, bz)
        local dx = bx-ax; local dy = by-ay; local dz = bz-az
        return math.sqrt(dx*dx + dy*dy + dz*dz)
    end

    local function getEnemyPos()
        return getPosition(enemy)
    end

    local function getPlayerPos()
        if not player then return nil end
        return getPosition(player)
    end

    local function setEnemyYaw(yaw)
        currentYaw = yaw
        if setRotation then
            setRotation(enemy, baseRotX, yaw, baseRotZ)
            local ex, ey, ez = getEnemyPos()
            -- re-teleport so Jolt physics body syncs rotation
            setPosition(enemy, ex, ey, ez)
        end
    end

    local function faceToward(tx, tz)
        local ex, ey, ez = getEnemyPos()
        local dx = tx - ex
        local dz = tz - ez
        if dx*dx + dz*dz > 1e-6 then
            setEnemyYaw(math.atan(dx, dz) + math.pi)
        end
    end

    -- Move enemy toward (tx,ty,tz) at speed. Returns true when arrived.
    local function moveToward(tx, ty, tz, speed, dt)
        local ex, ey, ez = getEnemyPos()
        local dx = tx-ex; local dy = ty-ey; local dz = tz-ez
        local nx, ny, nz, dist = normalize(dx, dy, dz)
        if dist <= ARRIVE_EPSILON then
            setPosition(enemy, tx, ty, tz)
            return true
        end
        local step = speed * dt
        if step >= dist then
            setPosition(enemy, tx, ty, tz)
            return true
        end
        setPosition(enemy, ex + nx*step, ey + ny*step, ez + nz*step)
        return false
    end

    local function distFromHome()
        local ex, ey, ez = getEnemyPos()
        return dist3(ex, ey, ez, home.x, home.y, home.z)
    end

    local function canCatchPlayer()
        if _G.PlayerState and _G.PlayerState.canBeCaught then
            return _G.PlayerState.canBeCaught()
        end
        return true
    end

    local animMoving = false

    local function setMoveAnim(moving)
        if animMoving == moving then return end
        animMoving = moving
        if not Animation then return end
        if moving then
            Animation.Play(enemy, ANIM_MOVE)
            Animation.SetLoop(enemy, true)
            Animation.SetSpeed(enemy, 1.0)
        else
            Animation.SetSpeed(enemy, 0.0)
        end
    end

    local function setCombatBGM(active)
        if _G.GlobalAudio and _G.GlobalAudio.setCombat then
            _G.GlobalAudio.setCombat(active)
        end
    end

    local function ensurePlayerCached()
        if player then return true end
        player = _G.PlayerEntity or findEntity("Player")
        if not player then return false end
        return true
    end

    -- =========================================================
    -- INIT
    -- =========================================================

    local function initOnce()
        -- Record home position from where the entity is placed in the editor
        local ex, ey, ez = getEnemyPos()
        home.x, home.y, home.z = ex, ey, ez

        local rx, ry, rz = getRotation(enemy)
        baseRotX, baseYaw, baseRotZ = rx, ry, rz
        currentYaw = baseYaw

        -- Cache respawn entity world position
        local respawnEnt = findEntity(RESPAWN_ENTITY_NAME)
        if respawnEnt then
            local rx2, ry2, rz2 = getWorldPosition(respawnEnt)
            respawnPos = { x = rx2, y = ry2, z = rz2 }
        else
            log("[CorridorChase] WARNING: respawn entity '" .. RESPAWN_ENTITY_NAME .. "' not found")
            respawnPos = { x = home.x, y = home.y, z = home.z }
        end

        triggered = false

        if Animation then
            Animation.SetSpeed(enemy, 0.0)
        end
    end

    -- Teleport enemy back to home, return to IDLE, re-arm trigger
    local function resetEnemy()
        setPosition(enemy, home.x, home.y, home.z)
        setEnemyYaw(baseYaw)
        setMoveAnim(false)
        setCombatBGM(false)
        state = STATE_IDLE
        stateTimer = 0.0
        triggered = false   -- re-arm the proximity trigger
    end

    -- Override checkpoint to the fixed respawn spot, then call onCaught
    local function catchPlayer()
        if not player then return end
        if not canCatchPlayer() then return end

        -- Set permanent checkpoint to the corridor respawn spot
        -- checkpointPos is a plain Lua table { x, y, z } read by onCaught
        if respawnPos and _G.PlayerState then
            _G.PlayerState.checkpointPos = { x = respawnPos.x, y = respawnPos.y, z = respawnPos.z }
        end

        if _G.PlayerState and _G.PlayerState.onCaught then
            _G.PlayerState.onCaught(player)
        end

        resetEnemy()
    end

    -- =========================================================
    -- STATE TRANSITIONS
    -- =========================================================

    local function enterState(newState)
        if state == newState then return end
        state = newState
        stateTimer = 0.0

        if newState == STATE_CAM_PAN then
            setMoveAnim(false)
            -- Fire the camera pan; onComplete callback transitions to ALERT_PAUSE.
            -- onComplete fires on the main Lua thread via registerUpdate, so
            -- stateTimer reset in the callback is safe.
            if _G.StartCameraPan then
                _G.StartCameraPan(CAM_WAYPOINTS, {
                    duration       = CAM_PAN_DURATION,
                    holdDuration   = CAM_HOLD_DURATION,
                    returnDuration = CAM_RETURN_DURATION,
                    easing         = "smoothstep",
                    onComplete     = function()
                        enterState(STATE_ALERT_PAUSE)
                    end,
                })
            else
                log("[CorridorChase] WARNING: _G.StartCameraPan not found, skipping pan")
                enterState(STATE_ALERT_PAUSE)
            end

        elseif newState == STATE_ALERT_PAUSE then
            setMoveAnim(false)
            -- Face toward player before charging
            if player then
                local px, py, pz = getPlayerPos()
                if px then faceToward(px, pz) end
            end

        elseif newState == STATE_CHASE then
            setMoveAnim(true)
            setCombatBGM(true)

        elseif newState == STATE_RETURN then
            setMoveAnim(true)
            setCombatBGM(false)
        end
    end

    -- =========================================================
    -- UPDATE
    -- =========================================================

    registerUpdate(function(dt)

        if not initialized then
            initOnce()
            initialized = true
        end

        if (not minimapTagged) and addTag then
            addTag(enemy, "Enemy")
            addTag(enemy, "danger")
            addTag(enemy, "corridor_chase")
            minimapTagged = true
        end

        if not ensurePlayerCached() then return end

        stateTimer = stateTimer + dt

        -- ==================================================
        -- IDLE: wait for player to enter trigger radius
        -- ==================================================
        if state == STATE_IDLE then
            if triggered then return end  -- trigger already fired, waiting for pan

            local ex, ey, ez = getEnemyPos()
            local px, py, pz = getPlayerPos()
            if not px then return end

            local d = dist3(ex, ey, ez, px, py, pz)
            if d <= TRIGGER_RADIUS then
                triggered = true

                -- snap camera to face away from the enemy so w/forward immediately runs away
                if _G.SetCameraYaw then
                    local awayYaw = math.atan(px - ex, pz - ez)
                    _G.SetCameraYaw(awayYaw)
                end

                enterState(STATE_CAM_PAN)
            end

        -- ==================================================
        -- CAM_PAN_WAIT: frozen, waiting for onComplete callback
        -- ==================================================
        elseif state == STATE_CAM_PAN then
            return  -- onComplete callback drives the transition

        -- ==================================================
        -- ALERT_PAUSE: brief freeze before charging
        -- ==================================================
        elseif state == STATE_ALERT_PAUSE then
            if stateTimer >= ALERT_PAUSE_TIME then
                enterState(STATE_CHASE)
            end

        -- ==================================================
        -- CHASE: charge straight at the player
        -- ==================================================
        elseif state == STATE_CHASE then
            local px, py, pz = getPlayerPos()
            if not px then return end

            -- Check max chase distance — give up if too far from home
            if distFromHome() >= MAX_CHASE_DISTANCE then
                enterState(STATE_RETURN)
                return
            end

            -- Move toward player
            faceToward(px, pz)
            moveToward(px, py, pz, CHASE_SPEED, dt)

        -- ==================================================
        -- RETURN: walk back to home
        -- ==================================================
        elseif state == STATE_RETURN then
            faceToward(home.x, home.z)
            local arrived = moveToward(home.x, home.y, home.z, RETURN_SPEED, dt)
            if arrived then
                resetEnemy()
            end
        end

    end)

    -- =========================================================
    -- COLLISION: catch player on physics contact (same as enemyGroundChase)
    -- =========================================================

    registerOnCollision(function(self, other)
        if not player then
            player = _G.PlayerEntity or findEntity("Player")
            if not player then return end
        end

        if other ~= player and self ~= player then return end

        -- Only catch while actively chasing
        if state ~= STATE_CHASE then return end

        if not canCatchPlayer() then return end

        -- Only catch when player is in front of the enemy (front-facing collision)
        local ex, ey, ez = getEnemyPos()
        local px, py, pz = getPosition(player)
        if px then
            local dx = px - ex
            local dz = pz - ez
            local len = math.sqrt(dx*dx + dz*dz)
            if len > 1e-6 then
                local fx = -math.sin(currentYaw)
                local fz = -math.cos(currentYaw)
                local dot = (dx/len)*fx + (dz/len)*fz
                if dot <= 0.0 then return end  -- player is behind or beside the enemy
            end
        end

        catchPlayer()
    end, player)

end  -- do block
