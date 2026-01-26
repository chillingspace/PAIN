-- PauseManager.lua
log("[PauseManager] Script LOADED at startup!")

local isPaused = false
local escWasPressed = false
local KEY_ESCAPE = 256
local sceneChange = false
local nextScene = nil

registerUpdate(function(dt)
    -- Handle ESC key for pause
    local escPressed = isKeyDown(KEY_ESCAPE)
    
    if escPressed and not escWasPressed then
        log("[PauseManager] ESC PRESSED - calling TogglePause")
        TogglePause()
    end
    
    escWasPressed = escPressed
    
    -- Handle scene changes (for restart button)
    if sceneChange then
        if nextScene == nil then
            return
        end
        
        changeScene(nextScene)
        sceneChange = false
        nextScene = nil
    end
end)

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

-- Continue button - unpause the game
_G_root.continue_button = function()
    log("[UI] Continue button pressed")
    if isPaused then
        TogglePause()
    end
end

-- Restart button - reload current scene
_G_root.restart_button = function()
    log("[UI] Restart button pressed")
    sceneChange = true
    nextScene = "Tutorial.scn"  -- Reloads the game scene
end

-- Quit to Main Menu button
_G_root.quit_button = function()
    log("[UI] Quit to menu button pressed")
    sceneChange = true
    nextScene = "mainmenu.scn"
end
