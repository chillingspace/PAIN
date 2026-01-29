
-- Default Configs
local config = {
    offX = 0.0,
    offY = 0.25,
    offZ = -0.5,
    
    mouseSensitivity = 0.002,
    touchSensitivity = 0.002,
    pitchLimit = 1.4 -- ~80 degrees
}

-- local baseOffset = { x = 0.0, y = 0.25, z = -0.5 }

local yaw   = 0.0   -- horizontal angle (radians)
local pitch = 0.2   -- slight downward tilt
local lastMouseX = nil
local lastMouseY = nil
local isInit = false

-- shared so movement can be camera-relative
_G.CameraState = { yaw = yaw, pitch = pitch }

registerUpdate(function(dt)
    -- EXIT EARLY IF PAUSED - stops camera movement
    if _G_root.gamePaused then
        return
    end

    local playerId = entityId

    if getCameraOffsets then
        local tx, ty, tz, rx, ry, rz = getCameraOffsets(playerId)
        
        -- Map C++ component fields to our Camera Logic:
        -- trans_offset.x -> Distance
        -- trans_offset.y -> Height
        -- trans_offset.z -> Look At Height
        -- rot_offset.y   -> Sensitivity
        
        -- Only override if values are set (not zero), otherwise use defaults
        if tx ~= 0 or ty ~= 0 or tz ~= 0 then 
            config.offX = tx
            config.offY = ty
            config.offZ = tz
        end

        if ry ~= 0 then 
            config.mouseSensitivity = ry 
            config.touchSensitivity = ry
        end

        -- Initialize Pitch from component once
        if not isInit and rx ~= 0 then
             pitch = rx
             isInit = true
        end
    end

    local px, py, pz = getPosition(playerId)

    local isMobile = (isAndroid ~= nil and isAndroid())

    ----------------------------------------------------------------
    -- 1. Update yaw/pitch from input
    ----------------------------------------------------------------
    if isMobile and getMobileLookDelta ~= nil then
        -- ANDROID: right-side drag → look delta from C++
        local dx, dy = getMobileLookDelta()
        yaw   = yaw   - dx * config.touchSensitivity      -- left/right
        pitch = pitch - dy * config.touchSensitivity      -- up/down
    else
        -- PC: use mouse position delta
        local mx, my = getMousePos()
        if lastMouseX ~= nil then
            local dx = mx - lastMouseX
            local dy = my - lastMouseY

            yaw   = yaw   - dx * config.mouseSensitivity  -- left/right
            pitch = pitch + dy * config.mouseSensitivity  -- up/down
        end
        lastMouseX, lastMouseY = mx, my
    end

    -- clamp pitch so we don't flip over
    if pitch >  config.pitchLimit then pitch =  config.pitchLimit end
    if pitch < -config.pitchLimit then pitch = -config.pitchLimit end


    -- COMPUTE POSITION BASED ON YAW AND PITCH
    local cosY, sinY = math.cos(yaw), math.sin(yaw)
    local cosP, sinP = math.cos(pitch), math.sin(pitch)

    -- Use values from config (which came from C++)
    local ox = config.offX
    local oy = config.offY
    local oz = config.offZ

    -- This rotates the offset vector up/down relative to the camera center
    local pitchX = ox 
    local pitchY = oy * cosP - oz * sinP
    local pitchZ = oy * sinP + oz * cosP

    -- This orbits the vector around the player
    local rx = pitchX * cosY + pitchZ * sinY
    local rz = -pitchX * sinY + pitchZ * cosY
    local ry = pitchY

    -- Final Position
    local cx = px + rx
    local cy = py + ry
    local cz = pz + rz

    -- Target: Look at Player Center
    -- looking at head
    local targetX = px
    local targetY = py + 0.1
    local targetZ = pz

    -- SET TRANSFORM
    cameraSetTransform(
        cx, cy, cz,
        targetX, targetY, targetZ,
        0.0, 1.0, 0.0
    )

    -- SHARE STATE
    _G.CameraState.yaw   = yaw
    _G.CameraState.pitch = pitch
end)
