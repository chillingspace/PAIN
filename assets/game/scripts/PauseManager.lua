-- PauseManager.lua
log("[PauseManager] Script LOADED at startup!")

-- Track current level globally
_G_root.CurrentLevelName = _G_root.CurrentLevelName or "Tutorial.scn"

local isPaused = false
local escWasPressed = false
local KEY_ESCAPE = 256

registerUpdate(function(dt)
    -- Handle ESC key for pause
    local escPressed = isKeyDown(KEY_ESCAPE)
    
    if escPressed and not escWasPressed then
        log("[PauseManager] ESC PRESSED - calling TogglePause")
        TogglePause()
    end
    
    escWasPressed = escPressed
end)

-- Make TogglePause globally accessible
function TogglePause()
    isPaused = not isPaused
    
    log("[PauseManager] Toggling pause: " .. tostring(isPaused))
    
    -- Broadcast to all scripts
    _G_root.gamePaused = isPaused
    
    -- Toggle UI layers
    setLayerEnabled(1, not isPaused)  -- Game UI layer
    setLayerEnabled(2, isPaused)       -- Pause menu layer
    
    -- Show/hide cursor based on pause state
    if isPaused then
        if showCursor then 
            showCursor()
            log("[PauseManager] Cursor shown")
        end
    else
        if hideCursor then 
            hideCursor()
            log("[PauseManager] Cursor hidden")
        end
    end
    
    log("[PauseManager] Layers toggled successfully")
end

-- Export to global scope so other scripts can call it
_G_root.TogglePause = TogglePause

-- Helper function to update current level name
function _G_root.SetCurrentLevel(sceneName)
    _G_root.CurrentLevelName = sceneName
    log("[PauseManager] Current level set to: " .. sceneName)
end
