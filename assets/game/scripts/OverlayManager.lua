-- OverlayManager.lua
log("[OverlayManager] Script LOADED at startup!")
hideCursor(false) -- hide cursor on startup

local isPaused = false
local escWasPressed = false
local KEY_ESCAPE = 256

registerUpdate(function(dt)
    -- Handle ESC key for pause
    local escPressed = isKeyDown(KEY_ESCAPE)
    
    if escPressed and not escWasPressed then
        log("[OverlayManager] ESC PRESSED - calling TogglePause")
        TogglePause()
    end
    
    escWasPressed = escPressed
end)

-- Make TogglePause globally accessible
function TogglePause()
    isPaused = not isPaused
    
    log("[OverlayManager] Toggling pause: " .. tostring(isPaused))
    
    -- Broadcast to all scripts
    SetGamePaused(isPaused) 

    -- Quit Overlay
    setLayerEnabled(3, isPaused)

    hideCursor(false)
    
    log("[OverlayManager] Layers toggled successfully")
end

-- Export to global scope so other scripts can call it
_G_root.TogglePause = TogglePause

-- Helper function to update current level name
-- Deprecated: Use getCurrentSceneName() from EngineAPI instead
-- function _G_root.SetCurrentLevel(sceneName)
--     _G_root.CurrentLevelName = sceneName
--     log("[PauseManager] Current level set to: " .. sceneName)
-- end
