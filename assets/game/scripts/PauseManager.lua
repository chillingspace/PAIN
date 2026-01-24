-- PauseManager.lua
log("[PauseManager] Script LOADED at startup!")

local isPaused = false
local escWasPressed = false

-- ESC key code for GLFW is 256, not 27
local KEY_ESCAPE = 256

registerUpdate(function(dt)
    -- Check if ESC key is currently down
    local escPressed = isKeyDown(KEY_ESCAPE)
    
    -- Detect edge trigger (key was just pressed this frame)
    if escPressed and not escWasPressed then
        log("[PauseManager] ESC PRESSED - calling TogglePause")
        TogglePause()
    end
    
    escWasPressed = escPressed
end)

function TogglePause()
    isPaused = not isPaused
    
    log("[PauseManager] Toggling pause: " .. tostring(isPaused))
    
    setLayerEnabled(1, not isPaused)  -- Game UI layer
    setLayerEnabled(2, isPaused)       -- Pause menu layer
    
    log("[PauseManager] Layers toggled successfully")
end

_G_root.ContinueButton_OnClick = function()
    log("[UI] Continue button pressed")
    if isPaused then
        TogglePause()
    end
end
