

-- thirdPersonCamera.lua

local playerId = nil

-- offset of camera relative to player in *player space*
local baseOffset = { x = 0.0, y = 3.0, z = -6.0 }

local yaw   = 0.0   -- horizontal angle (radians)
local pitch = 0.2   -- slight downward tilt
local mouseSensitivity = 0.002

local lastMouseX = nil
local lastMouseY = nil

registerUpdate(function(dt)
    if not playerId then
        local id = findEntity("Player")  
        if id == nil then
            return
        end
        playerId = id
    end

    local px, py, pz = getPosition(playerId)

    -- mouse shift, rotate camera
    local mx, my = getMousePos()
    if lastMouseX ~= nil then
        local dx = mx - lastMouseX
        local dy = my - lastMouseY

        yaw   = yaw   - dx * mouseSensitivity      -- left/right
        pitch = pitch - dy * mouseSensitivity      -- up/down

        -- clamp pitch a bit so you can’t flip over
        local limit = 1.0
        if pitch >  limit then pitch =  limit end
        if pitch < -limit then pitch = -limit end
    end
    lastMouseX, lastMouseY = mx, my

    -- build a rotated offset around the player (simple yaw+pitch)
    local cosY, sinY = math.cos(yaw), math.sin(yaw)
    local cosP, sinP = math.cos(pitch), math.sin(pitch)

    -- start from base offset
    local ox = baseOffset.x
    local oy = baseOffset.y
    local oz = baseOffset.z

    -- yaw around Y
    local rx = ox * cosY + oz * sinY
    local rz = -ox * sinY + oz * cosY
    local ry = oy

    -- pitch (tilt up/down) around X (very rough but ok for now)
    local ryp = ry * cosP - rz * sinP
    local rzp = ry * sinP + rz * cosP

    -- final camera position
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
end)
