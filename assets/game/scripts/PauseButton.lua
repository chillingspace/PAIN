-- PauseButton.lua
log("[PauseButton] Script loaded")

-- This script should be attached to the pause button entity

-- Called when the button is clicked (from UIButton component callback)
_G_root.pause_button = function()
    log("[PauseButton] Button clicked!")
    
    -- Check if TogglePause function exists (from PauseManager)
    if _G_root.TogglePause then
        _G_root.TogglePause()
        log("[PauseButton] Called TogglePause()")
    else
        log("[PauseButton] ERROR: TogglePause function not found!")
    end
end
