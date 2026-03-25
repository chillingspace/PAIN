-- ============================================================
-- Camera Pan State Machine
-- ============================================================

_G.CameraPan = _G.CameraPan or { active = false }

local pan = nil  -- nil = inactive

local function easeSmooth(t)
    return t * t * (3.0 - 2.0 * t)
end

local function lerpV(ax, ay, az, bx, by, bz, t)
    return ax + (bx - ax) * t,
           ay + (by - ay) * t,
           az + (bz - az) * t
end


-- Resolves a waypoint name into {px,py,pz, lx,ly,lz} in world space
local function resolveWaypoint(name)
    local e = findEntity(name)
    if not e then
        log("[CameraPan] WARNING: could not find waypoint entity '" .. name .. "'")
        return nil
    end
    local px, py, pz = getWorldPosition(e)
    local fx, fy, fz = getWorldForward(e)
    -- Guard: if forward is zero/nil (entity has no rotation data), fall back to world -Z
    if not fx or (fx*fx + fy*fy + fz*fz) < 1e-6 then
        log("[CameraPan] WARNING: waypoint '" .. name .. "' has zero/nil forward, using default (0,0,-1)")
        fx, fy, fz = 0.0, 0.0, -1.0
    end
    -- look target = world pos + world forward * 10
    return { px=px, py=py, pz=pz,
             lx=px+fx*10, ly=py+fy*10, lz=pz+fz*10 }
end

--[[
  StartCameraPan(waypointNames, options)
    waypointNames : array of entity name strings
    options       : {
        duration       = 2.0,  -- seconds to travel between each pair of waypoints
        holdDuration   = 0.0,  -- seconds to hold at EACH waypoint (override per-waypoint not needed)
        returnDuration = 1.5,  -- seconds to smoothly return to player follow-cam
        easing         = "smoothstep" | "linear",
        onComplete     = function() end,
    }
]]
_G.StartCameraPan = function(waypointNames, options)
    if _G.CameraPan.active then
        log("[CameraPan] Pan already active, ignoring new request")
        return
    end
    if not waypointNames or #waypointNames < 1 then
        log("[CameraPan] No waypoints provided")
        return
    end

    local opts = options or {}
    local waypoints = {}
    for _, name in ipairs(waypointNames) do
        local wp = resolveWaypoint(name)
        if wp then table.insert(waypoints, wp) end
    end

    if #waypoints < 1 then
        log("[CameraPan] No valid waypoints resolved")
        return
    end

    pan = {
        waypoints      = waypoints,
        current        = 1,          -- index of destination waypoint
        duration       = opts.duration       or 2.0,
        holdDuration   = opts.holdDuration   or 0.0,
        returnDuration = opts.returnDuration or 1.5,
        useSmooth      = (opts.easing ~= "linear"),
        onComplete     = opts.onComplete,
        elapsed        = 0.0,
        phase          = "travel",   -- "travel" | "hold" | "return"
        -- start position captured when pan begins (set on first update)
        startPx = nil,
        -- return phase start/end captured when return begins
        retSrcX=nil, retSrcY=nil, retSrcZ=nil,
        retSrcLx=nil, retSrcLy=nil, retSrcLz=nil,
        retElapsed = 0.0,
    }

    _G.CameraPan.active = true
    log("[CameraPan] Pan started with " .. #waypoints .. " waypoints")
end

_G.CancelCameraPan = function()
    if pan then
        pan = nil
        _G.CameraPan.active = false
        log("[CameraPan] Pan cancelled")
    end
end

-- ============================================================
-- Default Configs
local config = {
    offX = 0.0,
    offY = 0.25,
    offZ = -0.5,
    
    mouseSensitivity = 0.002,
    touchSensitivity = 0.002,
    pitchMax = 1.2,
    pitchMin = -0.3,

}

local DEFAULT_YAW = 0.0
local DEFAULT_PITCH = 0.2

local yaw   = DEFAULT_YAW
local pitch = DEFAULT_PITCH
local lastMouseX = nil
local lastMouseY = nil
local isInit = false
local firstFrame = true  -- Force reset on first frame

-- Camera
local smoothX, smoothY, smoothZ = nil, nil, nil
local smoothFactor = 15.0 -- Higher = tighter, Lower = smoother (and more lag)

local currentCamDist   = nil
local minSafeDist      = nil
local CAM_PULL_IN_SPEED  = 5.0  -- fast snap-in when a wall is hit
local CAM_PULL_OUT_SPEED =  2.0  -- slow ease-out when the wall clears
local CAM_FORGET_SPEED   =  1.0  -- how fast minSafeDist recovers after clearing (lower, less jitter)

-- Pipe traversal camera
local pipeCamX, pipeCamY, pipeCamZ = nil, nil, nil
local PIPE_CAM_BACK   = 2.0   -- units behind the player along the pipe axis
local PIPE_CAM_UP     = 0.35  -- camera raised above pipe axis
local PIPE_CAM_SMOOTH = 10.0  -- how fast the camera slides to the pipe view

_G.CameraState = { yaw = yaw, pitch = pitch }
local frozenCx, frozenCy, frozenCz = nil, nil, nil
local frozenTx, frozenTy, frozenTz = nil, nil, nil

local wasPaused = false
local framesSinceUnpause = 0

function _G.ResetThirdPersonCamera()
    firstFrame = true
    yaw = DEFAULT_YAW
    pitch = DEFAULT_PITCH
    lastMouseX = nil
    lastMouseY = nil
    currentCamDist = nil
    minSafeDist    = nil
    pipeCamX, pipeCamY, pipeCamZ = nil, nil, nil
    log("[Camera] Camera reset called externally")
end

log("[Camera] thirdPersonCamera.lua script loaded")
log("[Camera] entityId = " .. tostring(entityId))
if not entityId then
    log("[Camera] ERROR: entityId is nil! Camera script won't work!")
end

registerUpdate(function(dt)

    if not entityId then
        log("[Camera] ERROR: entityId is nil in update loop!")
        return
    end

    local playerId = entityId
    local px, py, pz = getPosition(playerId)

    if not px then
        log("[Camera] ERROR: getPosition returned nil for player " .. tostring(playerId))
        return
    end
    
    -- ============================================================
    -- Camera Pan Update
    -- ============================================================
    if pan then
        pan.elapsed = pan.elapsed + dt

        if pan.phase == "travel" then
            local wp = pan.waypoints[pan.current]
            local prevWp = pan.waypoints[pan.current - 1]

            -- Capture travel start position on first tick of each segment
            if pan.startPx == nil then
                -- Start from current camera position and look target
                pan.startPx = frozenCx or px
                pan.startPy = frozenCy or py
                pan.startPz = frozenCz or pz
                pan.startLx = frozenTx or px
                pan.startLy = frozenTy or (py + 0.1)
                pan.startLz = frozenTz or pz
            end

            local t = math.min(pan.elapsed / pan.duration, 1.0)
            if pan.useSmooth then t = easeSmooth(t) end

            local cx, cy, cz = lerpV(pan.startPx, pan.startPy, pan.startPz,
                                     wp.px, wp.py, wp.pz, t)
            local lx, ly, lz = lerpV(pan.startLx, pan.startLy, pan.startLz,
                                     wp.lx, wp.ly, wp.lz, t)

            cameraSetTransform(cx, cy, cz, lx, ly, lz, 0.0, 1.0, 0.0)
            frozenCx, frozenCy, frozenCz = cx, cy, cz
            frozenTx, frozenTy, frozenTz = lx, ly, lz

            if pan.elapsed >= pan.duration then
                -- Snap exactly to waypoint
                cameraSetTransform(wp.px, wp.py, wp.pz, wp.lx, wp.ly, wp.lz, 0.0, 1.0, 0.0)
                frozenCx, frozenCy, frozenCz = wp.px, wp.py, wp.pz
                frozenTx, frozenTy, frozenTz = wp.lx, wp.ly, wp.lz
                pan.elapsed = 0.0
                pan.startPx = nil

                if pan.holdDuration > 0.0 then
                    pan.phase = "hold"
                elseif pan.current < #pan.waypoints then
                    pan.current = pan.current + 1
                else
                    pan.phase = "return"
                end
            end

        elseif pan.phase == "hold" then
            local wp = pan.waypoints[pan.current]
            cameraSetTransform(wp.px, wp.py, wp.pz, wp.lx, wp.ly, wp.lz, 0.0, 1.0, 0.0)

            if pan.elapsed >= pan.holdDuration then
                pan.elapsed = 0.0
                if pan.current < #pan.waypoints then
                    pan.current = pan.current + 1
                    pan.phase = "travel"
                else
                    pan.phase = "return"
                end
            end

        elseif pan.phase == "return" then
            -- Capture return source on first tick
            if pan.retSrcX == nil then
                pan.retSrcX = frozenCx
                pan.retSrcY = frozenCy
                pan.retSrcZ = frozenCz
                pan.retSrcLx = frozenTx
                pan.retSrcLy = frozenTy
                pan.retSrcLz = frozenTz
                pan.retElapsed = 0.0
            end

            pan.retElapsed = pan.retElapsed + dt

            -- Compute where the follow-cam would be right now
            local cosY2, sinY2 = math.cos(yaw), math.sin(yaw)
            local cosP2, sinP2 = math.cos(pitch), math.sin(pitch)
            local pitchY2 = config.offY * cosP2 - config.offZ * sinP2
            local pitchZ2 = config.offY * sinP2 + config.offZ * cosP2
            local rx2 = config.offX * cosY2 + pitchZ2 * sinY2
            local rz2 = -config.offX * sinY2 + pitchZ2 * cosY2
            local targetCx = (smoothX or px) + rx2
            local targetCy = (smoothY or py) + pitchY2
            local targetCz = (smoothZ or pz) + rz2
            local targetLx = smoothX or px
            local targetLy = (smoothY or py) + 0.1
            local targetLz = smoothZ or pz

            local t = math.min(pan.retElapsed / pan.returnDuration, 1.0)
            if pan.useSmooth then t = easeSmooth(t) end

            local cx, cy, cz = lerpV(pan.retSrcX, pan.retSrcY, pan.retSrcZ,
                                     targetCx, targetCy, targetCz, t)
            local lx, ly, lz = lerpV(pan.retSrcLx, pan.retSrcLy, pan.retSrcLz,
                                     targetLx, targetLy, targetLz, t)

            cameraSetTransform(cx, cy, cz, lx, ly, lz, 0.0, 1.0, 0.0)
            frozenCx, frozenCy, frozenCz = cx, cy, cz
            frozenTx, frozenTy, frozenTz = lx, ly, lz

            if pan.retElapsed >= pan.returnDuration then
                -- Pan complete
                local cb = pan.onComplete
                pan = nil
                _G.CameraPan.active = false
                log("[CameraPan] Pan complete, returning to follow-cam")
                if cb then cb() end
            end
        end

        return  -- skip normal follow logic while panning
    end
    -- ============================================================

    -- FORCE CAMERA TO DEFAULT POSITION ON FIRST FRAME
    if firstFrame then
        log("[Camera] FIRST FRAME - Setting default camera position")
        
        -- Compute default camera position immediately
        local cosY = math.cos(DEFAULT_YAW)
        local sinY = math.sin(DEFAULT_YAW)
        local cosP = math.cos(DEFAULT_PITCH)
        local sinP = math.sin(DEFAULT_PITCH)
        
        local ox = config.offX
        local oy = config.offY
        local oz = config.offZ
        
        local pitchY = oy * cosP - oz * sinP
        local pitchZ = oy * sinP + oz * cosP
        
        local rx = pitchZ * sinY
        local rz = pitchZ * cosY
        local ry = pitchY
        
        local cx = px + rx
        local cy = py + ry
        local cz = pz + rz
        
        -- SET CAMERA IMMEDIATELY
        cameraSetTransform(
            cx, cy, cz,
            px, py + 0.1, pz,
            0.0, 1.0, 0.0
        )
        
        yaw = DEFAULT_YAW
        pitch = DEFAULT_PITCH
        lastMouseX = nil
        lastMouseY = nil
        framesSinceUnpause = 5
        firstFrame = false
        
        log("[Camera] Camera reset to default")
        return  -- Skip rest of update this frame
    end
    
    local isPaused = IsGamePaused() or false
    
    if wasPaused and not isPaused then
        log("[Camera] Detected unpause - resetting mouse tracking")
        lastMouseX = nil
        lastMouseY = nil
        framesSinceUnpause = 3
    end
    
    wasPaused = isPaused
    
    if isPaused then
        if frozenCx then
            cameraSetTransform(
                frozenCx, frozenCy, frozenCz,
                frozenTx, frozenTy, frozenTz,
                0.0, 1.0, 0.0
            )
        end
        return
    end
    
    if framesSinceUnpause > 0 then
        framesSinceUnpause = framesSinceUnpause - 1
        lastMouseX = nil
        lastMouseY = nil
    end

    if getCameraOffsets then
        local tx, ty, tz, rx, ry, rz = getCameraOffsets(playerId)
        
        if tx ~= 0 or ty ~= 0 or tz ~= 0 then 
            config.offX = tx
            config.offY = ty
            config.offZ = tz
        end

        if ry ~= 0 then 
            config.mouseSensitivity = ry 
            config.touchSensitivity = ry
        end

        if not isInit and rx ~= 0 then
             pitch = rx
             isInit = true
        end
    end

    local isMobile = (isAndroid ~= nil and isAndroid())

    if framesSinceUnpause == 0 then
        if isMobile and getMobileLookDelta ~= nil then
            local dx, dy = getMobileLookDelta()
            yaw   = yaw   - dx * config.touchSensitivity
            pitch = pitch - dy * config.touchSensitivity
        else
            local mx, my = getMousePos()
            if lastMouseX ~= nil then
                local dx = mx - lastMouseX
                local dy = my - lastMouseY

                yaw   = yaw   - dx * config.mouseSensitivity
                pitch = pitch + dy * config.mouseSensitivity
            end
            lastMouseX, lastMouseY = mx, my
        end
    end

     if pitch > config.pitchMax then pitch = config.pitchMax end
 if pitch < config.pitchMin then pitch = config.pitchMin end

    local cosY, sinY = math.cos(yaw), math.sin(yaw)
    local cosP, sinP = math.cos(pitch), math.sin(pitch)

    local ox = config.offX
    local oy = config.offY
    local oz = config.offZ

    local pitchX = ox 
    local pitchY = oy * cosP - oz * sinP
    local pitchZ = oy * sinP + oz * cosP

    local rx = pitchX * cosY + pitchZ * sinY
    local rz = -pitchX * sinY + pitchZ * cosY
    local ry = pitchY

    -- Initialize smooth vars if nil
    if not smoothX then 
        smoothX, smoothY, smoothZ = px, py, pz 
    end

    -- SMOOTHING: Interpolate towards the physics position
    -- Formula: current = current + (target - current) * speed * dt
    smoothX = smoothX + (px - smoothX) * smoothFactor * dt
    smoothY = smoothY + (py - smoothY) * smoothFactor * dt
    smoothZ = smoothZ + (pz - smoothZ) * smoothFactor * dt

    -- Pipe traversal: camera follows player along pipe axis, staying behind them
    local PS = _G.PlayerState
    if PS and PS.inPipe and PS.pipeEntrancePos then
        local ep = PS.pipeEntrancePos
        local dirX, dirY, dirZ = PS.pipeDirX, PS.pipeDirY, PS.pipeDirZ

        -- Project actual player pos onto the pipe axis to get traversal depth
        local progress = (px - ep.x) * dirX + (py - ep.y) * dirY + (pz - ep.z) * dirZ

        -- Camera sits PIPE_CAM_BACK units behind the player, raised above pipe axis.
        -- Offset tapers to 0 over the last PIPE_CAM_BACK units so the camera
        -- catches up to the exit instead of stopping short.
        local fadeOut = math.max(0.0, math.min(1.0, (PS.pipeLen - progress) / PIPE_CAM_BACK))
        local camT = progress - PIPE_CAM_BACK * fadeOut
        local tx = ep.x + dirX * camT
        local ty = ep.y + dirY * camT + PIPE_CAM_UP
        local tz = ep.z + dirZ * camT

        -- Look target: just ahead of the player along the pipe, same elevation as camera
        local lookT = progress + 0.5
        local lx = ep.x + dirX * lookT
        local ly = ep.y + dirY * lookT + PIPE_CAM_UP
        local lz = ep.z + dirZ * lookT

        -- Use Z-forward as up if pipe is nearly vertical
        local upX, upY, upZ = 0.0, 1.0, 0.0
        if math.abs(dirY) > 0.9 then upX, upY, upZ = 0.0, 0.0, 1.0 end

        -- Seed XZ from last camera position for smooth horizontal entry,
        -- snap Y immediately so camera never tilts during descent
        if not pipeCamX then
            pipeCamX = frozenCx or tx
            pipeCamY = ty
            pipeCamZ = frozenCz or tz
        end

        local t = 1.0 - math.exp(-PIPE_CAM_SMOOTH * dt)
        pipeCamX = pipeCamX + (tx - pipeCamX) * t
        pipeCamY = pipeCamY + (ty - pipeCamY) * t
        pipeCamZ = pipeCamZ + (tz - pipeCamZ) * t

        frozenCx, frozenCy, frozenCz = pipeCamX, pipeCamY, pipeCamZ
        frozenTx, frozenTy, frozenTz = lx, ly, lz

        cameraSetTransform(pipeCamX, pipeCamY, pipeCamZ, lx, ly, lz, upX, upY, upZ)
        return
    end
    -- Not in pipe: reset so next entry transitions from current position
    pipeCamX, pipeCamY, pipeCamZ = nil, nil, nil

    -- Desired camera offset distance from the smoothed player position
    local desiredDist = math.sqrt(rx*rx + ry*ry + rz*rz)

    -- Init on first valid frame
    if not currentCamDist then currentCamDist = desiredDist end
    if not minSafeDist    then minSafeDist    = desiredDist end

    -- Raw safe distance from the collision system this frame
    local safeDist = desiredDist
    if cameraGetPositionWithCollision and desiredDist > 0.001 then
        local colX, colY, colZ = cameraGetPositionWithCollision(
            smoothX, smoothY, smoothZ,
            smoothX + rx, smoothY + ry, smoothZ + rz,
            playerId
        )
        local cdx = colX - smoothX
        local cdy = colY - smoothY
        local cdz = colZ - smoothZ
        safeDist = math.sqrt(cdx*cdx + cdy*cdy + cdz*cdz)
    end

    local wallHit = safeDist < (desiredDist - 0.05)

    if wallHit then
        -- Wall present: use full two-layer system to prevent clipping
        local expForget = 1.0 - math.exp(-CAM_FORGET_SPEED * dt)
        minSafeDist = minSafeDist + (desiredDist - minSafeDist) * expForget
        if safeDist < minSafeDist then
            minSafeDist = safeDist
        end

        -- Snap in fast
        local t = 1.0 - math.exp(-CAM_PULL_IN_SPEED * dt)
        currentCamDist = currentCamDist + (minSafeDist - currentCamDist) * t
    else
        -- No wall: skip the gate entirely, single smooth ease out
        minSafeDist = desiredDist  -- reset gate so next wall hit starts clean
        local t = 1.0 - math.exp(-CAM_PULL_OUT_SPEED * dt)
        currentCamDist = currentCamDist + (desiredDist - currentCamDist) * t
    end

    -- final camera position along the desired direction at the smoothed distance
    local cx, cy, cz
    if desiredDist > 0.001 then
        local ux = rx / desiredDist
        local uy = ry / desiredDist
        local uz = rz / desiredDist
        cx = smoothX + ux * currentCamDist
        cy = smoothY + uy * currentCamDist
        cz = smoothZ + uz * currentCamDist
    else
        cx = smoothX + rx
        cy = smoothY + ry
        cz = smoothZ + rz
    end

    -- Always track camera position
    frozenCx, frozenCy, frozenCz = cx, cy, cz
    frozenTx, frozenTy, frozenTz = px, py + 0.1, pz

    cameraSetTransform(
        cx, cy, cz,
        smoothX, smoothY + 0.1, smoothZ, -- Target
        0.0, 1.0, 0.0
    )

    _G.CameraState.yaw   = yaw
    _G.CameraState.pitch = pitch
end)
