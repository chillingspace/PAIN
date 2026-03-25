-- cameraPan.lua
-- Attach to a trigger entity. Create child empty entities as waypoints and
-- position/rotate them in the editor to define the camera path.
--
-- Scene setup example:
--   MyCameraTrigger         <- attach this script here
--     CamWaypoint_My_1      <- empty entity, drag to desired camera position,
--     CamWaypoint_My_2         rotate to aim the camera direction (no script needed)
--
-- The camera will travel through each waypoint in order, then smoothly
-- return to the player follow-cam. Player input is frozen during the pan.

-- Names of the waypoint entities in the scene (in travel order)
local WAYPOINTS = {
    "CamWaypoint_My_1",
    "CamWaypoint_My_2",
   --CamWaypoint_My_3",
   --CamWaypoint_My_4",
}

-- Trigger: proximity radius around this entity
local TRIGGER_RADIUS = 3.0

-- Pan options
local PAN_OPTIONS = {
    duration       = 3.0, -- seconds to travel between each waypoint
    holdDuration   = 1.0, -- seconds to pause at each waypoint before continuing
    returnDuration = 1.5, -- seconds to smoothly ease back to the player follow-cam
    easing         = "smoothstep", 

    onComplete = function()
        -- e.g. trigger an audio, etc
        log("[CameraPan] Pan sequence complete")
    end,
}

-- ============================================================
-- TRIGGER LOGIC - can change later if trigger is not proximity based
-- ============================================================

local triggered = false

registerUpdate(function(dt)
    if triggered then return end
    if not _G.PlayerEntity then return end
    if TRIGGER_RADIUS <= 0 then return end

    local px, py, pz = getPosition(_G.PlayerEntity)
    local ex, ey, ez = getPosition(entityId)
    local dx, dy, dz = px - ex, py - ey, pz - ez
    local distSq = dx*dx + dy*dy + dz*dz

    if distSq <= TRIGGER_RADIUS * TRIGGER_RADIUS then
        triggered = true
        if _G.StartCameraPan then
            _G.StartCameraPan(WAYPOINTS, PAN_OPTIONS)
        else
            log("[CameraPan] ERROR: _G.StartCameraPan not found. Is thirdPersonCamera.lua loaded?")
        end
    end
end)
