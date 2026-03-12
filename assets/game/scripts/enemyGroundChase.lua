-- enemyGroundChase.lua
-- Ground enemy that:
--   patrols between two waypoints - like enemyPatrol.lua tagging
--   scans left/right with pauses at each endpoint
--   scans slowly while walking between points
--   smoothly turns to face next waypoint before walking
--   detects player in a forward vision cone
--   chases, searches, then returns to patrol
--   damages player on collision
--   reuses DetectionUI overlay/audio (hides the bar)
--   shows a 3D exclamation icon above itself

do
    local enemy = entityId
    local player = nil
    local minimapTagged = false
    local initialized = false

    -- =========================================================
    -- CONFIG
    -- =========================================================

    -- Vision
    local DETECTION_RADIUS       = 5.0
    local VISION_ANGLE_DEG       = 65.0   -- full cone width
    local LOST_SIGHT_GRACE       = 0.12

    -- Timing
    local ALERT_PAUSE_TIME       = 0.5
    local SEARCH_WAIT_TIME       = 1.5

    -- Chase / Search movement
    local CHASE_SPEED            = 1.5
    local RETURN_SPEED           = 1.2
    local MAX_CHASE_DISTANCE     = 10.0
    local ARRIVE_EPSILON         = 0.2

    -- Patrol movement
    local PATROL_SPEED           = 2.0
    local PATROL_INSET           = 1.5   -- so enemy doesnt slam walls

    -- Patrol waypoint tags 
    local START_TAG              = "enemy_patrol_start"
    local END_TAG                = "enemy_patrol_end"
    local SEARCH_RADIUS          = 15.0
    local PATROL_GROUPS          = { "A","B","C","D","E","F","G","H","I","J","K","L","M",
                                     "N","O","P","Q","R","S","T","U","V","W","X","Y","Z" }

    -- Endpoint scanning
    local ENDPOINT_SCAN_ANGLE_DEG = 80.0  -- +/- from travel direction
    local SCAN_TURN_SPEED_DEG    = 60.0   -- degrees/sec while scanning
    local SCAN_PAUSE_MIN         = 0.5    -- pause at each extreme (seconds)
    local SCAN_PAUSE_MAX         = 1.0
    local SCAN_CYCLES_MIN        = 1      -- random number of full L-R cycles
    local SCAN_CYCLES_MAX        = 2

    -- Walk scanning 
    local WALK_SCAN_SPEED        = 1.2    -- oscillation rate 
    local WALK_SCAN_ANGLE_DEG    = 25.0   -- +/- from movement direction

    -- Search scanning 
    local SEARCH_SCAN_ANGLE_DEG  = 120.0  -- +/- wider sweep to look around
    local SEARCH_TURN_SPEED_DEG  = 50.0   -- degrees/sec, slower and more deliberate
    local SEARCH_PAUSE_MIN       = 0.8    -- longer pauses, looking carefully
    local SEARCH_PAUSE_MAX       = 1.5
    local SEARCH_CYCLES_MIN      = 1      -- less cycles than endpoint scan
    local SEARCH_CYCLES_MAX      = 2

    -- Smooth turning
    local TURN_SPEED_DEG         = 120.0  -- degrees/sec for PATROL_TURN state

    -- Exclamation icon
    local ALERT_ICON_NAME        = "exclamation_alert"
    local ALERT_ICON_HEIGHT      = 1.0
    local SHOW_ICON_IN_ALERT     = true
    local SHOW_ICON_IN_CHASE     = true
    local SHOW_ICON_IN_SEARCH    = false
    local SHOW_ICON_IN_RETURN    = false

    -- Detection bar entities to hide, red overlay still visible
    local UI_DETECT_BAR_BG       = "UI_DetectBar_BG"
    local UI_DETECT_BAR_FILL_L   = "UI_DetectBar_Fill_L"
    local UI_DETECT_BAR_FILL_R   = "UI_DetectBar_Fill_R"
    local HIDE_UI_X              = -2000
    local HIDE_UI_Y              = -2000

    -- =========================================================
    -- STATE
    -- =========================================================

    -- Patrol states 
    local STATE_PATROL_WALK  = "PATROL_WALK"
    local STATE_PATROL_SCAN  = "PATROL_SCAN"
    local STATE_PATROL_TURN  = "PATROL_TURN"

    -- Combat states 
    local STATE_ALERT_PAUSE  = "ALERT_PAUSE"
    local STATE_CHASE        = "CHASE"
    local STATE_SEARCH       = "SEARCH"
    local STATE_RETURN_HOME  = "RETURN_HOME"

    local state = STATE_PATROL_WALK
    local stateTimer = 0.0
    local globalTime = 0.0

    -- Patrol waypoints
    local pointA = nil
    local pointB = nil
    local patrolTarget = nil   
    local goingToB = true

    -- Spawn position 
    local home = { x = 0.0, y = 0.0, z = 0.0 }
    local baseRotX, baseYaw, baseRotZ = 0.0, 0.0, 0.0

    -- Current facing yaw 
    local currentYaw = 0.0

    -- Endpoint scan sub-state
    local scanBaseYaw       = 0.0   -- direction enemy was traveling
    local scanCyclesLeft    = 0
    local scanPhase         = 0     -- 0=turn right, 1=pause right, 2=turn left, 3=pause left
    local scanTargetYaw     = 0.0
    local scanPauseTimer    = 0.0
    local scanPauseDuration = 0.0

    -- Walk scan
    local walkScanTimer = 0.0

    -- Chase / search
    local lastSeen = nil
    local lostSightTimer = 0.0
    local searchArrived = false
    local chaseOrigin = nil  -- position where the enemy first spotted the player

    local alertIcon = nil

    -- =========================================================
    -- HELPERS
    -- =========================================================

    local function clamp(v, lo, hi)
        if v < lo then return lo end
        if v > hi then return hi end
        return v
    end

    local function normalize(x, y, z)
        local len = math.sqrt(x * x + y * y + z * z)
        if len <= 1e-6 then return 0.0, 0.0, 0.0, 0.0 end
        return x / len, y / len, z / len, len
    end

    local function dist3(ax, ay, az, bx, by, bz)
        local dx = bx - ax
        local dy = by - ay
        local dz = bz - az
        return math.sqrt(dx * dx + dy * dy + dz * dz)
    end

    local function wrapAngle(a)
        while a >  math.pi do a = a - 2 * math.pi end
        while a < -math.pi do a = a + 2 * math.pi end
        return a
    end

    local function stepAngle(from, to, maxStep)
        local diff = wrapAngle(to - from)
        if math.abs(diff) <= maxStep then
            return to, true
        end
        if diff > 0 then
            return from + maxStep, false
        else
            return from - maxStep, false
        end
    end

    local function yawBetween(ax, az, bx, bz)
        return math.atan(bx - ax, bz - az)
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
            -- setRotation only updates LocalTransform, not the Jolt physics body
            -- re-teleport pos so teleportBodyToTransform syncs both pos+rot to Jolt
            local ex, ey, ez = getPosition(enemy)
            setPosition(enemy, ex, ey, ez)
        end
    end

    local function faceToward(tx, ty, tz)
        local ex, ey, ez = getEnemyPos()
        local dx = tx - ex
        local dz = tz - ez
        if dx * dx + dz * dz > 1e-6 then
            setEnemyYaw(math.atan(dx, dz))
        end
    end

    local function moveToward(tx, ty, tz, speed, dt)
        local ex, ey, ez = getEnemyPos()
        local dx = tx - ex
        local dy = ty - ey
        local dz = tz - ez
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
        setPosition(enemy, ex + nx * step, ey + ny * step, ez + nz * step)
        return false
    end

    local function distanceFromChaseOrigin()
        if not chaseOrigin then return 0.0 end
        local ex, ey, ez = getEnemyPos()
        return dist3(ex, ey, ez, chaseOrigin.x, chaseOrigin.y, chaseOrigin.z)
    end

    local function canCatchPlayer()
        if PlayerState and PlayerState.canBeCaught then
            return PlayerState.canBeCaught()
        end
        if _G.PlayerState and _G.PlayerState.canBeCaught then
            return _G.PlayerState.canBeCaught()
        end
        return true
    end

    local function damagePlayer()
        if not player then return end
        if not canCatchPlayer() then return end
        if PlayerState and PlayerState.onCaught then
            PlayerState.onCaught(player)
            return
        end
        if _G.PlayerState and _G.PlayerState.onCaught then
            _G.PlayerState.onCaught(player)
        end
    end

    local function setDetectionUIActive(active)
        if not DetectionUI then return end
        if active then
            if DetectionUI.begin then DetectionUI.begin(enemy, false) end
        else
            if DetectionUI.cancel then DetectionUI.cancel(enemy) end
        end
    end

    local function hideDetectionBarOnly()
        local barBG    = findEntity(UI_DETECT_BAR_BG)
        local barFillL = findEntity(UI_DETECT_BAR_FILL_L)
        local barFillR = findEntity(UI_DETECT_BAR_FILL_R)
        if barBG    then set2DPosition(barBG,    HIDE_UI_X, HIDE_UI_Y) end
        if barFillL then set2DPosition(barFillL, HIDE_UI_X, HIDE_UI_Y) end
        if barFillR then set2DPosition(barFillR, HIDE_UI_X, HIDE_UI_Y) end
    end

    -- vision cone light
    local function updateVisionLight()
        if not hasLight or not hasLight(enemy) then return end
        local fx = math.sin(currentYaw)
        local fz = math.cos(currentYaw)
        setLightDirection(enemy, fx, -0.15, fz)   -- slight downward tilt; auto-normalized by engine

        if state == STATE_ALERT_PAUSE or state == STATE_CHASE then
            setLightIntensity(enemy, 1.0, 0.1, 0.1)    -- red: alerted
        elseif state == STATE_SEARCH then
            setLightIntensity(enemy, 1.0, 0.55, 0.0)   -- orange: searching
        else
            setLightIntensity(enemy, 0.6, 0.6, 0.8)    -- pale blue: patrolling
        end
    end

    -- alert icon helpers
    local function updateAlertIcon(visible)
        if not alertIcon then return end
        if not setVisibility then return end
        setVisibility(alertIcon, visible)
        if visible then
            local ex, ey, ez = getEnemyPos()
            setPosition(alertIcon, ex, ey + ALERT_ICON_HEIGHT, ez)
        end
    end

    local function refreshAlertIconForState()
        if not alertIcon then return end
        local show = false
        if state == STATE_ALERT_PAUSE then show = SHOW_ICON_IN_ALERT
        elseif state == STATE_CHASE   then show = SHOW_ICON_IN_CHASE
        elseif state == STATE_SEARCH  then show = SHOW_ICON_IN_SEARCH
        elseif state == STATE_RETURN_HOME then show = SHOW_ICON_IN_RETURN
        end
        updateAlertIcon(show)
    end

    local function updateLastSeen()
        if not player then return end
        local px, py, pz = getPlayerPos()
        if not px then return end
        lastSeen = { x = px, y = py, z = pz }
    end

    local function playerInVisionCone()
        if not player then return false end
        if not canCatchPlayer() then return false end

        local ex, ey, ez = getEnemyPos()
        local px, py, pz = getPlayerPos()
        if not px then return false end

        local vx, vy, vz, dist = normalize(px - ex, py - ey, pz - ez)
        if dist <= 0.0 or dist > DETECTION_RADIUS then return false end

        -- Forward direction from current yaw
        local fx = math.sin(currentYaw)
        local fz = math.cos(currentYaw)

        local dot = vx * fx + vz * fz
        dot = clamp(dot, -1.0, 1.0)
        local angleDeg = math.deg(math.acos(dot))
        return angleDeg <= (VISION_ANGLE_DEG * 0.5)
    end

    -- Distance-only check (no cone). Used once the enemy is already alerted.
    local function playerInRange()
        if not player then return false end
        if not canCatchPlayer() then return false end

        local ex, ey, ez = getEnemyPos()
        local px, py, pz = getPlayerPos()
        if not px then return false end

        local dist = dist3(ex, ey, ez, px, py, pz)
        return dist <= DETECTION_RADIUS
    end

    local function ensurePlayerCached()
        if player then return true end
        player = _G.PlayerEntity or findEntity("Player")
        if not player then return false end
        if PlayerState and PlayerState.init then
            PlayerState.init(player)
        elseif _G.PlayerState and _G.PlayerState.init then
            _G.PlayerState.init(player)
        end
        return true
    end

    local function tryCacheAlertIcon()
        if alertIcon or ALERT_ICON_NAME == "" then return end
        alertIcon = findEntity(ALERT_ICON_NAME)
        if alertIcon and setVisibility then
            setVisibility(alertIcon, false)
        end
    end

    -- =========================================================
    -- WAYPOINT DISCOVERY (from enemyPatrol.lua)
    -- =========================================================

    local function findNearestByTag(ox, oy, oz, tag, maxDistSq)
        local candidates = getEntitiesByTag(tag)
        if not candidates or #candidates == 0 then return nil end
        local best = nil
        local bestDistSq = maxDistSq
        for _, e in ipairs(candidates) do
            local x, y, z = getPosition(e)
            local dx = x - ox
            local dy = y - oy
            local dz = z - oz
            local d2 = dx*dx + dy*dy + dz*dz
            if d2 <= bestDistSq then
                bestDistSq = d2
                best = { x = x, y = y, z = z }
            end
        end
        return best
    end

    local function discoverWaypoints()
        local ex, ey, ez = getEnemyPos()
        local startWp, endWp

        -- 1. Check named patrol group tags
        for _, group in ipairs(PATROL_GROUPS) do
            if hasTag(enemy, "patrol_group_" .. group) then
                local sTag = START_TAG .. "_" .. group
                local eTag = END_TAG   .. "_" .. group
                local c = getEntitiesByTag(sTag)
                if c and #c > 0 then
                    local x, y, z = getPosition(c[1])
                    startWp = { x = x, y = y, z = z }
                end
                c = getEntitiesByTag(eTag)
                if c and #c > 0 then
                    local x, y, z = getPosition(c[1])
                    endWp = { x = x, y = y, z = z }
                end
                break
            end
        end

        -- 2. Fallback: nearest in radius
        if not startWp then
            startWp = findNearestByTag(ex, ey, ez, START_TAG, SEARCH_RADIUS * SEARCH_RADIUS)
        end
        if not endWp then
            endWp = findNearestByTag(ex, ey, ez, END_TAG, SEARCH_RADIUS * SEARCH_RADIUS)
        end

        -- 3. Final fallback
        if not startWp then startWp = { x = ex, y = ey, z = ez } end
        if not endWp   then endWp   = { x = startWp.x + 5.0, y = startWp.y, z = startWp.z } end

        pointA = { x = startWp.x, y = startWp.y, z = startWp.z }
        pointB = { x = endWp.x,   y = endWp.y,   z = endWp.z   }

        -- Inset endpoints so the enemy doesn't walk into walls
        local vx = pointB.x - pointA.x
        local vz = pointB.z - pointA.z
        local lenSq = vx * vx + vz * vz
        if lenSq > 0.0001 then
            local len = math.sqrt(lenSq)
            if len > PATROL_INSET * 3.0 then
                local inset = math.min(PATROL_INSET, len * 0.4)
                local ux = vx / len
                local uz = vz / len
                pointA.x = pointA.x + ux * inset
                pointA.z = pointA.z + uz * inset
                pointB.x = pointB.x - ux * inset
                pointB.z = pointB.z - uz * inset
            end
        end

        patrolTarget = goingToB and pointB or pointA
    end

    -- =========================================================
    -- STATE TRANSITIONS
    -- =========================================================

    local function beginPatrolScan(travelYaw)
        state = STATE_PATROL_SCAN
        stateTimer = 0.0

        scanBaseYaw = travelYaw
        scanCyclesLeft = math.random(SCAN_CYCLES_MIN, SCAN_CYCLES_MAX)

        -- Start by scanning to the right (+) side
        scanPhase = 0
        scanTargetYaw = scanBaseYaw + math.rad(ENDPOINT_SCAN_ANGLE_DEG)
        scanPauseTimer = 0.0
        scanPauseDuration = 0.0

        setDetectionUIActive(false)
        refreshAlertIconForState()
    end

    local function beginPatrolTurn()
        state = STATE_PATROL_TURN
        stateTimer = 0.0
        setDetectionUIActive(false)
        refreshAlertIconForState()
    end

    local function beginPatrolWalk()
        state = STATE_PATROL_WALK
        stateTimer = 0.0
        walkScanTimer = 0.0
        setDetectionUIActive(false)
        refreshAlertIconForState()
    end

    local function enterState(newState)
        if state == newState then return end

        state = newState
        stateTimer = 0.0

        if newState == STATE_ALERT_PAUSE then
            setDetectionUIActive(true)
            hideDetectionBarOnly()
            updateLastSeen()
            lostSightTimer = 0.0
            searchArrived = false
            -- Record where the enemy was when it spotted the player
            local ex, ey, ez = getEnemyPos()
            chaseOrigin = { x = ex, y = ey, z = ez }

        elseif newState == STATE_CHASE then
            setDetectionUIActive(true)
            hideDetectionBarOnly()
            lostSightTimer = 0.0
            searchArrived = false

        elseif newState == STATE_SEARCH then
            setDetectionUIActive(false)
            lostSightTimer = 0.0
            searchArrived = false

        elseif newState == STATE_RETURN_HOME then
            setDetectionUIActive(false)
            lostSightTimer = 0.0
            searchArrived = false
        end

        refreshAlertIconForState()
    end

    -- =========================================================
    -- INIT
    -- =========================================================

    local function initOnce()
        local ex, ey, ez = getEnemyPos()
        home.x, home.y, home.z = ex, ey, ez

        local rx, ry, rz = getRotation(enemy)
        baseRotX, baseYaw, baseRotZ = rx, ry, rz
        currentYaw = baseYaw

        -- Derive icon name from patrol group tag so each enemy finds its own icon.
        if ALERT_ICON_NAME ~= "" then
            for _, group in ipairs(PATROL_GROUPS) do
                if hasTag(enemy, "patrol_group_" .. group) then
                    ALERT_ICON_NAME = "exclamation_alert_" .. group
                    break
                end
            end
        end

        discoverWaypoints()
        tryCacheAlertIcon()
    end

    -- =========================================================
    -- UPDATE
    -- =========================================================

    registerUpdate(function(dt)
        globalTime = globalTime + dt

        if not initialized then
            initOnce()
            initialized = true
        end

        if (not minimapTagged) and addTag then
            addTag(enemy, "Enemy")
            addTag(enemy, "danger")
            addTag(enemy, "ground_chase")
            minimapTagged = true
        end

        if not ensurePlayerCached() then return end

        tryCacheAlertIcon()
        refreshAlertIconForState()

        -- Keep detection bar hidden when this enemy owns the UI
        if DetectionUI
            and DetectionUI.active
            and DetectionUI.sourceEnemy == enemy
            and DetectionUI.autoConfirmHit == false then
            hideDetectionBarOnly()
        end

        stateTimer = stateTimer + dt

        updateVisionLight()

        local SCAN_TURN_SPEED = math.rad(SCAN_TURN_SPEED_DEG)
        local TURN_SPEED      = math.rad(TURN_SPEED_DEG)

        -- =============================================================
        -- PATROL_WALK: move toward target waypoint, slow scan while walking
        -- =============================================================
        if state == STATE_PATROL_WALK then
            if playerInVisionCone() then
                updateLastSeen()
                enterState(STATE_ALERT_PAUSE)
                return
            end

            local ex, ey, ez = getEnemyPos()
            local tx, ty, tz = patrolTarget.x, patrolTarget.y, patrolTarget.z
            local dx = tx - ex
            local dz = tz - ez
            local distSq = dx*dx + dz*dz

            if distSq < ARRIVE_EPSILON * ARRIVE_EPSILON then
                -- Arrived at waypoint — enter scan facing travel direction
                local travelYaw = currentYaw -- yaw we were walking with
                beginPatrolScan(travelYaw)
                return
            end

            -- Move
            local dist = math.sqrt(distSq)
            local nx, nz = dx / dist, dz / dist
            local step = PATROL_SPEED * dt
            if step >= dist then
                setPosition(enemy, tx, ty, tz)
                local travelYaw = math.atan(nx, nz)
                beginPatrolScan(travelYaw)
                return
            end

            setPosition(enemy, ex + nx * step, ey, ez + nz * step)

            -- Face movement direction (no scanning while walking)
            local moveYaw = math.atan(nx, nz)
            setEnemyYaw(moveYaw)

        -- =============================================================
        -- PATROL_SCAN: at endpoint, scan left/right with pauses
        -- =============================================================
        elseif state == STATE_PATROL_SCAN then
            if playerInVisionCone() then
                updateLastSeen()
                enterState(STATE_ALERT_PAUSE)
                return
            end

            -- Scan phases:
            --   0 = rotating toward right extreme
            --   1 = pausing at right extreme
            --   2 = rotating toward left extreme
            --   3 = pausing at left extreme

            if scanPhase == 0 then
                -- Rotate toward right extreme
                local newYaw, reached = stepAngle(currentYaw, scanTargetYaw, SCAN_TURN_SPEED * dt)
                setEnemyYaw(newYaw)
                if reached then
                    scanPhase = 1
                    scanPauseDuration = SCAN_PAUSE_MIN + math.random() * (SCAN_PAUSE_MAX - SCAN_PAUSE_MIN)
                    scanPauseTimer = 0.0
                end

            elseif scanPhase == 1 then
                -- Pause at right
                scanPauseTimer = scanPauseTimer + dt
                if scanPauseTimer >= scanPauseDuration then
                    scanPhase = 2
                    scanTargetYaw = scanBaseYaw - math.rad(ENDPOINT_SCAN_ANGLE_DEG)
                end

            elseif scanPhase == 2 then
                -- Rotate toward left extreme
                local newYaw, reached = stepAngle(currentYaw, scanTargetYaw, SCAN_TURN_SPEED * dt)
                setEnemyYaw(newYaw)
                if reached then
                    scanPhase = 3
                    scanPauseDuration = SCAN_PAUSE_MIN + math.random() * (SCAN_PAUSE_MAX - SCAN_PAUSE_MIN)
                    scanPauseTimer = 0.0
                end

            elseif scanPhase == 3 then
                -- Pause at left
                scanPauseTimer = scanPauseTimer + dt
                if scanPauseTimer >= scanPauseDuration then
                    scanCyclesLeft = scanCyclesLeft - 1
                    if scanCyclesLeft > 0 then
                        -- Another cycle: back to right
                        scanPhase = 0
                        scanTargetYaw = scanBaseYaw + math.rad(ENDPOINT_SCAN_ANGLE_DEG)
                    else
                        -- Done scanning — swap patrol target and turn
                        goingToB = not goingToB
                        patrolTarget = goingToB and pointB or pointA
                        beginPatrolTurn()
                    end
                end
            end

        -- =============================================================
        -- PATROL_TURN: smoothly rotate to face next waypoint
        -- =============================================================
        elseif state == STATE_PATROL_TURN then
            if playerInVisionCone() then
                updateLastSeen()
                enterState(STATE_ALERT_PAUSE)
                return
            end

            local ex, ey, ez = getEnemyPos()
            local targetYaw = yawBetween(ex, ez, patrolTarget.x, patrolTarget.z)
            local newYaw, reached = stepAngle(currentYaw, targetYaw, TURN_SPEED * dt)
            setEnemyYaw(newYaw)

            if reached then
                beginPatrolWalk()
            end

        -- =============================================================
        -- ALERT_PAUSE: enemy spotted player, pauses briefly before chasing
        -- Uses distance check (not cone) because the enemy is already alerted
        -- =============================================================
        elseif state == STATE_ALERT_PAUSE then
            if DetectionUI and DetectionUI.active
                and DetectionUI.sourceEnemy == enemy
                and DetectionUI.autoConfirmHit == false then
                hideDetectionBarOnly()
            end

            if not canCatchPlayer() then
                enterState(STATE_RETURN_HOME)
                return
            end

            -- Always face the player while alerted (no cone gate — we already know they're there)
            local px, py, pz = getPlayerPos()
            if px then
                updateLastSeen()
                faceToward(px, py, pz)
            end

            if stateTimer >= ALERT_PAUSE_TIME then
                if playerInRange() then
                    updateLastSeen()
                    enterState(STATE_CHASE)
                else
                    enterState(STATE_SEARCH)
                end
                return
            end

        -- =============================================================
        -- CHASE: pursue the player, use distance check to maintain pursuit
        -- =============================================================
        elseif state == STATE_CHASE then
            if DetectionUI and DetectionUI.active
                and DetectionUI.sourceEnemy == enemy
                and DetectionUI.autoConfirmHit == false then
                hideDetectionBarOnly()
            end

            if not canCatchPlayer() then
                enterState(STATE_SEARCH)
                return
            end

            if distanceFromChaseOrigin() >= MAX_CHASE_DISTANCE then
                enterState(STATE_RETURN_HOME)
                return
            end

            -- Once chasing, track by distance (not cone) — enemy is committed
            if playerInRange() then
                updateLastSeen()
                lostSightTimer = 0.0
            else
                lostSightTimer = lostSightTimer + dt
                if lostSightTimer >= LOST_SIGHT_GRACE then
                    enterState(STATE_SEARCH)
                    return
                end
            end

            local px, py, pz = getPlayerPos()
            if px then
                moveToward(px, py, pz, CHASE_SPEED, dt)
                faceToward(px, py, pz)
            end

        -- =============================================================
        -- SEARCH: go to last-seen position, then scan thoroughly
        -- Reuses scanPhase/scanBaseYaw/etc. (safe since PATROL_SCAN
        -- and SEARCH are mutually exclusive states)
        -- =============================================================
        elseif state == STATE_SEARCH then
            if playerInVisionCone() then
                updateLastSeen()
                enterState(STATE_ALERT_PAUSE)
                return
            end

            local SEARCH_TURN_SPEED = math.rad(SEARCH_TURN_SPEED_DEG)

            -- Phase 1: walk to last-seen position
            if (not searchArrived) and lastSeen then
                local arrived = moveToward(lastSeen.x, lastSeen.y, lastSeen.z, RETURN_SPEED, dt)
                faceToward(lastSeen.x, lastSeen.y, lastSeen.z)
                if arrived then
                    searchArrived = true
                    -- Set up phased scan at arrival facing direction
                    scanBaseYaw = currentYaw
                    scanCyclesLeft = math.random(SEARCH_CYCLES_MIN, SEARCH_CYCLES_MAX)
                    scanPhase = 0
                    scanTargetYaw = scanBaseYaw + math.rad(SEARCH_SCAN_ANGLE_DEG)
                    scanPauseTimer = 0.0
                    scanPauseDuration = 0.0
                end

            -- Phase 2: scan in place (same 4-phase system as endpoint scan)
            else
                if scanPhase == 0 then
                    local newYaw, reached = stepAngle(currentYaw, scanTargetYaw, SEARCH_TURN_SPEED * dt)
                    setEnemyYaw(newYaw)
                    if reached then
                        scanPhase = 1
                        scanPauseDuration = SEARCH_PAUSE_MIN + math.random() * (SEARCH_PAUSE_MAX - SEARCH_PAUSE_MIN)
                        scanPauseTimer = 0.0
                    end

                elseif scanPhase == 1 then
                    scanPauseTimer = scanPauseTimer + dt
                    if scanPauseTimer >= scanPauseDuration then
                        scanPhase = 2
                        scanTargetYaw = scanBaseYaw - math.rad(SEARCH_SCAN_ANGLE_DEG)
                    end

                elseif scanPhase == 2 then
                    local newYaw, reached = stepAngle(currentYaw, scanTargetYaw, SEARCH_TURN_SPEED * dt)
                    setEnemyYaw(newYaw)
                    if reached then
                        scanPhase = 3
                        scanPauseDuration = SEARCH_PAUSE_MIN + math.random() * (SEARCH_PAUSE_MAX - SEARCH_PAUSE_MIN)
                        scanPauseTimer = 0.0
                    end

                elseif scanPhase == 3 then
                    scanPauseTimer = scanPauseTimer + dt
                    if scanPauseTimer >= scanPauseDuration then
                        scanCyclesLeft = scanCyclesLeft - 1
                        if scanCyclesLeft > 0 then
                            scanPhase = 0
                            scanTargetYaw = scanBaseYaw + math.rad(SEARCH_SCAN_ANGLE_DEG)
                        else
                            -- Done searching — give up and return
                            lastSeen = nil
                            enterState(STATE_RETURN_HOME)
                            return
                        end
                    end
                end
            end

        -- =============================================================
        -- RETURN_HOME: go to nearest patrol waypoint, resume patrol
        -- =============================================================
        elseif state == STATE_RETURN_HOME then
            if playerInVisionCone() then
                updateLastSeen()
                enterState(STATE_ALERT_PAUSE)
                return
            end

            -- Return to nearest patrol waypoint
            local ex, ey, ez = getEnemyPos()
            local distA = dist3(ex, ey, ez, pointA.x, pointA.y, pointA.z)
            local distB = dist3(ex, ey, ez, pointB.x, pointB.y, pointB.z)

            local returnTarget
            if distA <= distB then
                returnTarget = pointA
                goingToB = true  -- after arriving at A, next patrol goes to B
            else
                returnTarget = pointB
                goingToB = false -- after arriving at B, next patrol goes to A
            end

            local arrived = moveToward(returnTarget.x, returnTarget.y, returnTarget.z, RETURN_SPEED, dt)
            faceToward(returnTarget.x, returnTarget.y, returnTarget.z)

            if arrived then
                -- Resume patrol: scan at this endpoint, then walk to the other
                patrolTarget = goingToB and pointB or pointA
                local travelYaw = yawBetween(returnTarget.x, returnTarget.z,
                                             patrolTarget.x, patrolTarget.z)
                beginPatrolScan(travelYaw)
            end
        end
    end)

    -- =========================================================
    -- COLLISION
    -- =========================================================

    registerOnCollision(function(self, other)
        if not player then
            player = findEntity("Player")
            if not player then return end
        end

        if other ~= player and self ~= player then return end

        -- Only damage when actively chasing
        if state ~= STATE_CHASE then return end

        if PlayerState and PlayerState.canBeCaught and not PlayerState.canBeCaught() then
            return
        end

        -- Only damage when player is in front of the enemy (front-facing collision)
        local ex, ey, ez = getEnemyPos()
        local px, py, pz = getPosition(player)
        if px then
            local dx = px - ex
            local dz = pz - ez
            local len = math.sqrt(dx * dx + dz * dz)
            if len > 1e-6 then
                local fx = math.sin(currentYaw)
                local fz = math.cos(currentYaw)
                local dot = (dx / len) * fx + (dz / len) * fz
                if dot <= 0.0 then return end  -- player is behind or beside the enemy
            end
        end

        if PlayerState and PlayerState.onCaught then
            PlayerState.onCaught(player)
        elseif _G.PlayerState and _G.PlayerState.onCaught then
            _G.PlayerState.onCaught(player)
        end
    end, player)
end