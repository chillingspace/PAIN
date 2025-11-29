

-- thirdPersonCamera.lua

local playerId = nil

-- offset of camera relative to player in *player space*
local baseOffset = { x = 0.0, y = 3.0, z = -6.0 }

local yaw   = 0.0   -- horizontal angle (radians)
local pitch = 0.2   -- slight downward tilt
local mouseSensitivity = 0.002
local touchSensitivity = 0.002 -- rightside drag on android

local lastMouseX = nil
local lastMouseY = nil

-- shared so movement can be camera-relative
_G.CameraState = _G.CameraState or { yaw = yaw, pitch = pitch }

registerUpdate(function(dt)
    if not playerId then
        local id = findEntity("Player")  
        if id == nil then
            return
        end
        playerId = id
    end

    local px, py, pz = getPosition(playerId)

    local isMobile = (isAndroid ~= nil and isAndroid())

    ----------------------------------------------------------------
    -- 1. Update yaw/pitch from input
    ----------------------------------------------------------------
    if isMobile and getMobileLookDelta ~= nil then
        -- ANDROID: right-side drag → look delta from C++
        local dx, dy = getMobileLookDelta()
        yaw   = yaw   - dx * touchSensitivity      -- left/right
        pitch = pitch - dy * touchSensitivity      -- up/down
    else
        -- PC: use mouse position delta
        local mx, my = getMousePos()
        if lastMouseX ~= nil then
            local dx = mx - lastMouseX
            local dy = my - lastMouseY

            yaw   = yaw   - dx * mouseSensitivity  -- left/right
            pitch = pitch - dy * mouseSensitivity  -- up/down
        end
        lastMouseX, lastMouseY = mx, my
    end

    -- clamp pitch so we don't flip over
    local limit = 1.0
    if pitch >  limit then pitch =  limit end
    if pitch < -limit then pitch = -limit end

    ----------------------------------------------------------------
    -- 2. Compute camera position around player 
    ----------------------------------------------------------------
    local cosY, sinY = math.cos(yaw), math.sin(yaw)
    local cosP, sinP = math.cos(pitch), math.sin(pitch)

    local ox = baseOffset.x
    local oy = baseOffset.y
    local oz = baseOffset.z

    -- yaw around Y axis
    local rx = ox * cosY + oz * sinY
    local rz = -ox * sinY + oz * cosY
    local ry = oy

    -- pitch around X axis
    local ryp = ry * cosP - rz * sinP
    local rzp = ry * sinP + rz * cosP

    -- final camera position (player + rotated offset)
    local cx = px + rx
    local cy = py + ryp
    local cz = pz + rzp

    -- aim camera at player (slightly above feet)
    local targetX = px
    local targetY = py + 1.0
    local targetZ = pz

    cameraSetTransform(
        cx, cy, cz,
        targetX, targetY, targetZ,
        0.0, 1.0, 0.0       -- world up
    )

    -- share yaw/pitch for movement
    _G.CameraState.yaw   = yaw
    _G.CameraState.pitch = pitch
end)
