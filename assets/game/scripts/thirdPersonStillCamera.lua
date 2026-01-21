
-- offset of camera relative to player in *player space*
local baseOffset = { x = 0.1, y = 0.0, z = 0.3 }

local yaw   = 0.0   -- horizontal angle (radians)
local pitch = -0.2   -- slight downward tilt

-- shared so movement can be camera-relative
_G.CameraState = _G.CameraState or { yaw = yaw, pitch = pitch }

registerUpdate(function(dt)
    local playerId = entityId
    local px, py, pz = getPosition(playerId)

    local isMobile = (isAndroid ~= nil and isAndroid())

    ----------------------------------------------------------------
    -- 1. Compute camera position around player 
    ----------------------------------------------------------------
    local cosY, sinY = math.cos(yaw), math.sin(yaw)
    local cosP, sinP = math.cos(pitch), math.sin(pitch)

    local ox = baseOffset.x
    local oy = baseOffset.y
    local oz = baseOffset.z

    -- STEP 1: Pitch first (Rotate around Local X axis)
    -- We rotate the base offset vector by pitch. 
    -- This ensures "Up" is always "Up" relative to the camera view.
    local pitchX = ox 
    local pitchY = oy * cosP - oz * sinP
    local pitchZ = oy * sinP + oz * cosP

    -- STEP 2: Yaw second (Rotate around Global Y axis)
    -- Now we spin that pitched vector around the player.
    local rx = pitchX * cosY + pitchZ * sinY
    local rz = -pitchX * sinY + pitchZ * cosY
    local ry = pitchY

    -- final camera position (player + rotated offset)
    local cx = px + rx
    local cy = py + ry
    local cz = pz + rz

    -- aim camera at player (Looking at head/center is usually better than feet)
    local targetX = px
    local targetY = py -- + 1.0 (Optional: Look at head instead of feet)
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
