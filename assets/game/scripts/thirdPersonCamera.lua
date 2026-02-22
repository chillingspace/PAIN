-- Default Configs
local config = {
    offX = 0.0,
    offY = 0.25,
    offZ = -0.5,
    
    mouseSensitivity = 0.002,
    touchSensitivity = 0.002,
    pitchLimit = 1.4
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

    if pitch >  config.pitchLimit then pitch =  config.pitchLimit end
    if pitch < -config.pitchLimit then pitch = -config.pitchLimit end

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

    local cx = smoothX + rx
    local cy = smoothY + ry
    local cz = smoothZ + rz

    -- Apply camera collision (uses active camera's settings from editor)
    if cameraResolveCollision then
        cx, cy, cz = cameraResolveCollision(
            cx, cy, cz,                    -- proposed position
            frozenCx or cx, frozenCy or cy, frozenCz or cz  -- current/last valid position
        )
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
