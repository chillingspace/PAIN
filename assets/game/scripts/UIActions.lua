-- UIActions.lua

local G = _G_root

-- scene defaults
G.CurrentLevelName    = G.CurrentLevelName   or "game/scenes/Tutorial.scn"
-- G.FirstLevelScene     = G.FirstLevelScene    or "game/scenes/Level1.scn"
G.TutorialSceneName   = G.TutorialSceneName  or "game/scenes/Tutorial.scn"
-- G.MainMenuSceneName   = G.MainMenuSceneName  or "game/scenes/mainmenu.scn"
-- G.HowToPlaySceneName  = G.HowToPlaySceneName or "game/scenes/howtoplay.scn"
-- G.HowToPlaySceneName2 = G.HowToPlaySceneName2 or "game/scenes/howtoplay2.scn"
-- G.CutSceneName        = G.CutSceneName        or "game/scenes/cutscene.scn"
-- G.CreditsSceneName    = G.CreditsSceneName    or "game/scenes/credits.scn"
-- G.CreditsSceneName2   = G.CreditsSceneName2   or "game/scenes/credits2.scn"

-- -- Placeholder for next level
G.NextLevelName       = G.NextLevelName      or "game/scenes/Tutorial.scn"

local layers = {
    DEFAULT = 0,
    MAINMENU = 1,
    QUIT = 2,
    SETTINGS = 3,
    AUDIO = 4,
    GRAPHICS = 5,
    CONTROLS = 6,
    HOWTOPLAY_1 = 7,
    HOWTOPLAY_2 = 8,
    CREDITS_1 = 9,
    CREDITS_2 = 10
}

-- ==================== GRAPHICS SETTINGS ====================
G.GraphicsSettings = G.GraphicsSettings or {
    displayMode = "windowed"
}

local function updateGraphicsModeDisplay()
    local e = findEntity("display_state")
    if not e then
        printLog("[UI] display_state not found")
        return
    end

    if G.GraphicsSettings.displayMode == "fullscreen" then
        setTexture(e, getImageID("game/textures/fullscreen_text.png"))
    else
        setTexture(e, getImageID("game/textures/windowed_text.png"))
    end
end


-- ==================== CONTROLS / KEY BINDINGS ====================
local KB_DEFAULTS = {
    forward  = "W",
    left     = "A",
    right    = "D",
    backward = "S",
    jump     = "SPACE",
    hide     = "E",
}

local KB_ACTION_NAMES = { "forward", "left", "right", "backward", "jump", "hide" }

-- Valid keys that can be used for rebinding (excludes F-keys, system keys, arrows)
local KB_VALID_KEYS = {}
for c = string.byte("A"), string.byte("Z") do
    KB_VALID_KEYS[string.char(c)] = true
end
for i = 0, 9 do
    KB_VALID_KEYS[tostring(i)] = true
end
KB_VALID_KEYS["SPACE"]  = true
KB_VALID_KEYS["TAB"]    = true
KB_VALID_KEYS["LSHIFT"] = true
KB_VALID_KEYS["RSHIFT"] = true

-- Init global table
_G.KeyBindings = _G.KeyBindings or {}
local KB = _G.KeyBindings

for _, action in ipairs(KB_ACTION_NAMES) do
    if KB[action] == nil then
        KB[action] = KB_DEFAULTS[action]
    end
end

KB._defaults    = KB_DEFAULTS
KB._actionNames = KB_ACTION_NAMES
KB._validKeys   = KB_VALID_KEYS

-- Load saved bindings from settings.cfg
local function loadBindings()
    for _, action in ipairs(KB_ACTION_NAMES) do
        local key = "controls_" .. action
        if settingsLoad then
            local saved = settingsLoad(key)
            if saved and saved ~= "" then
                KB[action] = saved
                printLog("[KeyBindings] Loaded " .. action .. " = " .. saved)
            end
        end
    end
end

-- Save all bindings to settings.cfg
function KB.saveAll()
    for _, action in ipairs(KB_ACTION_NAMES) do
        local key = "controls_" .. action
        local val = KB[action] or KB_DEFAULTS[action]
        if settingsSave then
            settingsSave(key, val)
        end
    end
    printLog("[KeyBindings] All bindings saved")
end

-- Reset all bindings to defaults
function KB.resetAll()
    for _, action in ipairs(KB_ACTION_NAMES) do
        KB[action] = KB_DEFAULTS[action]
    end
    KB.saveAll()
    printLog("[KeyBindings] All bindings reset to defaults")
end

-- Rebinding state
KB._pendingRebind = nil
KB._pendingEntity = nil

function KB.isValidKey(keyName)
    return KB_VALID_KEYS[keyName] == true
end

function KB.getDisplayName(keyName)
    if     keyName == "SPACE"  then return "SPACE"
    elseif keyName == "LSHIFT" then return "L-SHIFT"
    elseif keyName == "RSHIFT" then return "R-SHIFT"
    elseif keyName == "TAB"    then return "TAB"
    elseif keyName == ""       then return "---"
    else   return keyName
    end
end

function KB.findActionByKey(keyName)
    for _, action in ipairs(KB_ACTION_NAMES) do
        if KB[action] == keyName then
            return action
        end
    end
    return nil
end

function KB.startRebind(action, uiEntity)
    KB._pendingRebind = action
    KB._pendingEntity = uiEntity
    printLog("[KeyBindings] Waiting for key input to rebind: " .. action)
end

function KB.tryApplyRebind(keyName)
    if not KB._pendingRebind then return false end

    local action = KB._pendingRebind

    if not KB.isValidKey(keyName) then
        printLog("[KeyBindings] Invalid key for rebind: " .. tostring(keyName))
        return true
    end

    -- Orphan method: unbind any other action using this key
    local existingAction = KB.findActionByKey(keyName)
    if existingAction and existingAction ~= action then
        KB[existingAction] = ""
        printLog("[KeyBindings] Unbound " .. existingAction .. " (was " .. keyName .. ")")

        local orphanEntity = findEntity("bind_" .. existingAction)
        if orphanEntity and setUIText then
            setUIText(orphanEntity, KB.getDisplayName(""))
        end
    end

    -- Apply new binding
    KB[action] = keyName
    printLog("[KeyBindings] Rebound " .. action .. " to " .. keyName)

    -- Update UI text on the button entity
    if KB._pendingEntity and setUIText then
        setUIText(KB._pendingEntity, KB.getDisplayName(keyName))
    end

    KB.saveAll()
    KB._pendingRebind = nil
    KB._pendingEntity = nil
    return true
end

function KB.cancelRebind()
    if KB._pendingRebind then
        printLog("[KeyBindings] Rebind cancelled for: " .. KB._pendingRebind)
        KB._pendingRebind = nil
        KB._pendingEntity = nil
    end
end

-- Load saved bindings on init
loadBindings()

printLog("[KeyBindings] Init: forward=" .. KB.forward ..
    " left=" .. KB.left ..
    " right=" .. KB.right ..
    " backward=" .. KB.backward ..
    " jump=" .. KB.jump ..
    " hide=" .. KB.hide)


-- ==================== AUDIO-AWARE SCENE CHANGE ====================
-- Helper to do scene transitions with audio fade-out
local function changeSceneWithAudioFade(scenePath)
    if not scenePath then
        printLog("[UI] Error: scenePath is nil")
        return
    end
    
    -- Use GlobalAudio fade if available, otherwise direct change
    if _G.GlobalAudio and _G.GlobalAudio.changeSceneWithFade then
        printLog("[UI] changeSceneWithAudioFade -> " .. scenePath)
        _G.GlobalAudio.changeSceneWithFade(scenePath)
    elseif changeScene then
        printLog("[UI] changeScene (no fade) -> " .. scenePath)
        changeScene(scenePath)
    else
        printLog("[UI] changeScene not available")
    end
end

local Layers = {
    DEFAULT = 0,
    UI = 1,
    PAUSE = 2,
    TERRAIN = 3,
    QUIT = 4,
    RESTART = 5,
    GAME_OVER = 6,
    GAME_WIN = 7,
}

-- ==================== UI SFX ====================
local UI_SFX_CLICKS = {
    "game/audio/sfx/ui/clicks/click 1.wav",
    "game/audio/sfx/ui/clicks/click 2.wav",
    "game/audio/sfx/ui/clicks/click 3.wav",
    "game/audio/sfx/ui/clicks/click 4.wav"
}
local UI_SFX_MENU_OPEN = "game/audio/sfx/ui/menu/menu open sfx.mp3"
local UI_SFX_MENU_CLOSE = "game/audio/sfx/ui/menu/menu close sfx.mp3"

-- SFX Volumes
local VOL_UI_CLICK = -3.0
local VOL_UI_OPEN = 0.0
local VOL_UI_CLOSE = 0.0

-- Play random UI click sound (non-spatial)
local function playUIClick()
    if audioPlayRandomSFX then
        audioPlayRandomSFX(UI_SFX_CLICKS, VOL_UI_CLICK)
    end
end

-- Play menu open/close sounds
local function playMenuOpen()
    if audioPlaySFX then audioPlaySFX(UI_SFX_MENU_OPEN, VOL_UI_OPEN) end
end

local function playMenuClose()
    if audioPlaySFX then audioPlaySFX(UI_SFX_MENU_CLOSE, VOL_UI_CLOSE) end
end

-- Store current level name globally (For Game_PlayerState)
if G and G.CurrentLevelName then
    _G.CurrentLevelName = G.CurrentLevelName
end

if G and G.NextLevelName then
    _G.NextLevelName = G.NextLevelName
end

-- print("[UIActions] running, _G_root=", _G_root)
-- print("[UIActions] before default, CurrentLevelName=", _G_root and _G_root.CurrentLevelName)

local PlayerState = _G.PlayerState

local function showGameEndOverlay(result)
    if not setLayerEnabled then
        printLog("[UI] setLayerEnabled not available")
        return
    end

    -- hide gameplay/pause stuff if needed
    setLayerEnabled(Layers.PAUSE, false) -- pause menu off 

    -- Hide both end screens first 
    setLayerEnabled(Layers.GAME_OVER, false)
    setLayerEnabled(Layers.GAME_WIN,  false)

    -- Enable Cursor
    hideCursor(false)

    if result == "win" then
        setLayerEnabled(Layers.GAME_WIN, true)
        printLog("[UI] Showing GAME WIN overlay")
    else
        setLayerEnabled(Layers.GAME_OVER, true)
        printLog("[UI] Showing GAME OVER overlay")
    end

    printLog("[UI] Game end overlay shown, result=" .. tostring(result))
end


local handlers = {
    ----------------------------------------------------------------------
    -- GAME END MENUS
    ----------------------------------------------------------------------
    game_End = function(buttonEntity, payload)
        -- payload expected: win or lose
        showGameEndOverlay(payload)
    end,

    ----------------------------------------------------------------------
    -- GAMEPLAY BUTTONS
    ----------------------------------------------------------------------
    game_Jump = function(buttonEntity, payload)
        if _G.PlayerInput and _G.PlayerInput.requestJump then
            _G.PlayerInput.requestJump()
            return
        end

        if PlayerState and PlayerState.isHidden and PlayerState.isHidden() then
            return
        end

        printLog("[UI] game_Jump pressed, but PlayerInput.requestJump is missing")
    end,

    game_Move = function(buttonEntity, payload)
        if IsGamePaused() then
            if _G_root.Joystick_OnDrag then
                _G_root.Joystick_OnDrag(0.0, 0.0)
            end
            return
        end
        
        local dirX, dirY = 0.0, 0.0
        
        if type(payload) == "string" then
            local values = {}
            for val in string.gmatch(payload, "[^,]+") do
                table.insert(values, tonumber(val) or 0.0)
            end
            dirX = values[1] or 0.0
            dirY = values[2] or 0.0
        elseif type(payload) == "table" then
            dirX = payload.dirX or payload.x or 0.0
            dirY = payload.dirY or payload.y or 0.0
        end
        
        if _G_root.Joystick_OnDrag then
            _G_root.Joystick_OnDrag(dirX, dirY)
        else
            printLog("[UI] game_Move pressed, but Joystick_OnDrag is missing")
        end
    end,

    game_Hide = function(buttonEntity, payload)
        if PlayerState and PlayerState.onHideButton then
            PlayerState.onHideButton()
        else
            printLog("[UI] game_Hide pressed, but PlayerState.onHideButton is missing")
        end
    end,

    game_Collect = function(buttonEntity, payload)
        if PlayerState and PlayerState.onCollectButton then
            PlayerState.onCollectButton()
        else
            printLog("[UI] game_Collect pressed, but PlayerState.onCollectButton is missing")
        end
    end,

    ----------------------------------------------------------------------
    -- PAUSE MENU
    ----------------------------------------------------------------------
    goto_Pause = function(buttonEntity, payload)
        -- Android touch button callback to pause the game
        -- playMenuOpen()  -- Play menu open SFX audio dont even fking work 
        playUIClick()
        if _G_root.TogglePause then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end

            _G_root.TogglePause()
            printLog("[UI] goto_Pause (Android) -> called TogglePause()")
        else
            printLog("[UI] goto_Pause pressed, but TogglePause is not available")
        end
    end,

    pause_Resume = function(buttonEntity, payload)
        if IsGamePaused() then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
            end
            playUIClick()
            _G_root.TogglePause()
            setLayerEnabled(1, not _G_root.IsGamePaused())
            printLog("[UI] pause_Resume -> called TogglePause()")
        else
            printLog("[UI] pause_Resume pressed, but TogglePause is not available")
        end
    end,

    pause_Restart = function(buttonEntity, payload)
        -- Show restart confirmation popup
        printLog("[UI] pause_Restart -> showing restart confirmation")
        playUIClick()
        if setLayerEnabled then
            setLayerEnabled(Layers.RESTART, true)  -- Show RestartOverlay layer (layer 5)
            setLayerEnabled(Layers.PAUSE, false) -- Hide PauseMenu

            setLayerEnabled(Layers.GAME_OVER, false)
            setLayerEnabled(Layers.GAME_WIN, false)

            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            printLog("[UI] RestartOverlay (layer 5) shown")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,

    pause_Quit = function(buttonEntity, payload)
        -- Show Quit confirmation popup
        printLog("[UI] pause_Quit -> showing quit confirmation")
        playUIClick()
        if setLayerEnabled then
            setLayerEnabled(Layers.QUIT, true)  -- Show Quit Confirmation layer (layer 6)
            setLayerEnabled(Layers.PAUSE, false) -- Hide PauseMenu

            setLayerEnabled(Layers.GAME_OVER, false)
            setLayerEnabled(Layers.GAME_WIN, false)

            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            printLog("[UI] QuitOverlay (layer 6) shown")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,

    restart_Confirm = function(buttonEntity, payload)
        -- User pressed YES - restart level
        printLog("[UI] restart_Confirm -> restarting")
        playUIClick()
        -- Unpause first
        if  _G_root.IsGamePaused() then
            _G_root.SetGamePaused(false) 
            printLog("[UI] Game unpaused before scene change")
        end
        
        -- Hide restart overlay
        if setLayerEnabled then
            setLayerEnabled(Layers.RESTART, false)
            printLog("[UI] RestartOverlay hidden")
        end
        
        -- Stop game-over audio before scene change
        if PlayerState and PlayerState.stopGameOverAudio then
            PlayerState.stopGameOverAudio()
        end

        -- Restart Scene
        if changeScene then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
            end
            
            local curr = ""
            if getCurrentSceneName then curr = getCurrentSceneName() end
            if curr == "" then curr = G.CurrentLevelName end
            
            printLog("[UI] restart_Confirm -> changeScene("..tostring(curr)..")")
            changeSceneWithAudioFade(curr)
        else
            printLog("[UI] restart_Confirm pressed, but changeScene is not bound")
        end
    end,

    restart_Cancel = function(buttonEntity, payload)
        -- User pressed NO - close popup
        printLog("[UI] restart_Cancel -> closing restart confirmation")
        playUIClick()
        if setLayerEnabled then
            setLayerEnabled(Layers.RESTART, false)

            if PlayerState and PlayerState.gameEnded then
                if PlayerState.gameWon then
                    setLayerEnabled(Layers.GAME_WIN, true)
                else
                    setLayerEnabled(Layers.GAME_OVER, true)
                end
            elseif _G_root.IsGamePaused then
                -- Return to pause menu if paused
                setLayerEnabled(Layers.PAUSE, true) -- Return to Pause Menu
            end
            

            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            printLog("[UI] Overlay (layer 5) hidden - returning to pause menu")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,

    ----------------------------------------------------------------------
    -- Quit Confirmation Overlay
    ----------------------------------------------------------------------
    quit_Confirm = function(buttonEntity, payload)
        -- User pressed YES - quit to main menu
        printLog("[UI] quit_Confirm -> returning to main menu")
        playUIClick()
        -- Unpause first
        if IsGamePaused() then
            SetGamePaused(false) 
            printLog("[UI] Game unpaused before scene change")
        end
        
        -- Hide quit overlay
        if setLayerEnabled then
            setLayerEnabled(Layers.QUIT, false)
            printLog("[UI] QuitOverlay hidden")
        end
        
        -- Stop game-over audio before scene change
        if PlayerState and PlayerState.stopGameOverAudio then
            PlayerState.stopGameOverAudio()
        end

        -- Load main menu 
        printLog("[UI] quit_Confirm -> changeScene(" .. G.MainMenuSceneName .. ")")
        if changeScene then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
                
            end
            changeSceneWithAudioFade(G.MainMenuSceneName)
        else
            printLog("[UI] quit_Confirm pressed, but changeScene is not bound")
        end
    end,

    quit_Cancel = function(buttonEntity, payload)
        -- User pressed NO - close popup
        printLog("[UI] quit_Cancel -> closing quit confirmation")
        playUIClick()
        if setLayerEnabled then
            setLayerEnabled(Layers.QUIT, false)
            -- setLayerEnabled(Layers.PAUSE, true)

            if PlayerState and PlayerState.gameEnded then
                if PlayerState.gameWon then
                    setLayerEnabled(Layers.GAME_WIN, true)
                else
                    setLayerEnabled(Layers.GAME_OVER, true)
                end
            elseif _G_root.IsGamePaused then
                -- Return to pause menu if paused
                setLayerEnabled(Layers.PAUSE, true) -- Return to Pause Menu
            end

            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
                
            end
            printLog("[UI] QuitOverlay (layer 4) hidden - returning to pause menu")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,

    ----------------------------------------------------------------------
    -- MAIN MENU
    ----------------------------------------------------------------------
    menu_QuitGame = function(buttonEntity, payload)
        playUIClick()
        local isMobile = (isAndroid ~= nil and isAndroid())
        if isMobile then
           hideCursor(true)
        end
        --SetGamePaused(not IsGamePaused()) 
        _G_root.TogglePause()
        
        setLayerEnabled(1, false)
        setLayerEnabled(2, true)
        printLog("[UI] cutscene_Menu_Button -> called TogglePause()")
    end,


    ----------------------------------------------------------------------
    -- CUT SCENE
    ----------------------------------------------------------------------
    cutscene_Open_Menu = function(buttonEntity, payload)
        -- Android touch button callback to pause the game
        playUIClick()
        local isMobile = (isAndroid ~= nil and isAndroid())
        if isMobile then
            hideCursor(false)
        end
        SetGamePaused(true)

        setLayerEnabled(2, true)

        printLog("[UI] cutscene_Open_Menu (Android) -> called TogglePause()")
    end,

    cutscene_Close_Menu = function(buttonEntity, payload)
        if IsGamePaused() then
            playUIClick()
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
            end
            --SetGamePaused(not IsGamePaused()) 
            SetGamePaused(false)
            
            setLayerEnabled(2, false)
            printLog("[UI] cutscene_Close_Menu -> called TogglePause()")
        else
            printLog("[UI] cutscene_Close_Menu pressed, but TogglePause is not available")
        end
    end,

    cutscene_Menu_Quit = function(buttonEntity, payload)
        if IsGamePaused() then
            playUIClick()
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
            end
            --SetGamePaused(not IsGamePaused()) 
            SetGamePaused(true)
            
            setLayerEnabled(3, true)
            printLog("[UI] cutscene_Menu_Button -> called TogglePause()")
        else
            printLog("[UI] cutscene_Menu_Button pressed, but TogglePause is not available")
        end
    end,

    cutscene_Quit_Confirm = function(buttonEntity, payload)
        -- User pressed YES - quit to main menu
        printLog("[UI] cutscene_Quit_Confirm -> returning to main menu")
        
        -- Unpause first
        if IsGamePaused() then
            playUIClick()
            SetGamePaused(false) 
            printLog("[UI] Game unpaused before scene change")
        end
        
        -- Hide quit overlay
        if setLayerEnabled then
            playUIClick()
            setLayerEnabled(Layers.QUIT, false)
            printLog("[UI] QuitOverlay hidden")
        end
        
        -- Load main menu 
        printLog("[UI] cutscene_Quit_Confirm -> changeScene(" .. G.MainMenuSceneName .. ")")
        if changeScene then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                playUIClick()
                hideCursor(true)
                
            end
            changeSceneWithAudioFade(G.MainMenuSceneName)
        else
            printLog("[UI] cutscene_Quit_Confirm pressed, but changeScene is not bound")
        end
    end,

    cutscene_Quit_Cancel = function(buttonEntity, payload)
        -- User pressed NO - close popup
        printLog("[UI] cutscene_Quit_Cancel -> closing quit confirmation")
        
        if setLayerEnabled then
            playUIClick()
            SetGamePaused(false)

            setLayerEnabled(3, false)
            setLayerEnabled(2, false) -- Return to Menu
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            --_G_root.TogglePause()
            printLog("[UI] QuitOverlay (layer 5) hidden - returning to menu")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,


    ----------------------------------------------------------------------
    -- MAIN MENU QUIT CONFIRMATION
    ----------------------------------------------------------------------
    mainmenu_Quit_Confirm = function(buttonEntity, payload)
        if quitApplication then
            playUIClick()
            printLog("[UI] cutscene_Quit_Confirm -> quitApplication()")
            quitApplication()
        else
            printLog("[UI] cutscene_Quit_Confirm pressed (quitapplication() not bound; implement in EngineAPI)")
        end
    end,

    mainmenu_Quit_Cancel = function(buttonEntity, payload)
        -- User pressed NO - close popup
        printLog("[UI] mainmenu_Quit_Cancel -> closing quit confirmation")
        
        if setLayerEnabled then
            playUIClick()
            setLayerEnabled(1, true)
            setLayerEnabled(2, false) -- Return to Menu
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            _G_root.TogglePause()

            printLog("[UI] QuitOverlay (layer 5) hidden - returning to menu")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,

    ----------------------------------------------------------------------
    -- LOAD SCENE (Generic)
    ----------------------------------------------------------------------
    LoadScene = function(buttonEntity, payload)
        if not payload or payload == "" then
            printLog("[UI] LoadScene error: payload is nil or empty")
            return
        end

        printLog("[UI] LoadScene -> " .. tostring(payload))
        playUIClick()
        
        -- Unpause if paused
        if IsGamePaused() then
            SetGamePaused(false)
        end
        
        -- Handle cursor visibility based on target scene type
        local isMobile = (isAndroid ~= nil and isAndroid())
        if isMobile then
            local sceneName = string.lower(payload)
            -- Menu scenes should show cursor, game scenes should hide cursor
            local isMenuScene = string.find(sceneName, "menu") or 
                                 string.find(sceneName, "howtoplay") or 
                                 string.find(sceneName, "credits") or
                                 string.find(sceneName, "cutscene") or
                                 string.find(sceneName, "settings")
            
            if isMenuScene then
                hideCursor(false)  -- Show cursor for menus
            else
                hideCursor(true)   -- Hide cursor for gameplay
            end
        end
        
        -- Stop game-over audio before scene change
        if PlayerState and PlayerState.stopGameOverAudio then
            PlayerState.stopGameOverAudio()
        end

        changeSceneWithAudioFade(payload)
    end,


    ----------------------------------------------------------------------
    -- HOW TO PLAY
    ----------------------------------------------------------------------

    howtoplay_Right = function(buttonEntity, payload)
        printLog("[UI] howtoplay_Right -> how to play page 2")
        
        if setLayerEnabled then

            setLayerEnabled(1, false)
            setLayerEnabled(2, true)
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            --_G_root.TogglePause()
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,


    howtoplay_Left = function(buttonEntity, payload)
        printLog("[UI] howtoplay_Left -> how to play page 1")
        
        if setLayerEnabled then

            setLayerEnabled(1, true)
            setLayerEnabled(2, false)
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            --_G_root.TogglePause()
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,



    ----------------------------------------------------------------------
    -- GRAPHICS SETTINGS
    ----------------------------------------------------------------------
    graphics_Left = function(buttonEntity, payload)
        playUIClick()

        if _G.GraphicsSettings.displayMode == "fullscreen" then
            _G.GraphicsSettings.displayMode = "windowed"
        else
            _G.GraphicsSettings.displayMode = "fullscreen"
        end

        -- Apply fullscreen/windowed mode (PC only)
        if setFullscreen then
            setFullscreen(_G.GraphicsSettings.displayMode == "fullscreen")
        end

        -- Save to settings file (disabled: fullscreen should not persist across launches)
        -- if settingsSave then
        --     settingsSave("gfx_displaymode", _G.GraphicsSettings.displayMode)
        -- end

        updateGraphicsModeDisplay()
        printLog("[UI] graphics_Left -> " .. tostring(_G.GraphicsSettings.displayMode))
    end,

    graphics_Right = function(buttonEntity, payload)
        playUIClick()

        if _G.GraphicsSettings.displayMode == "windowed" then
            _G.GraphicsSettings.displayMode = "fullscreen"
        else
            _G.GraphicsSettings.displayMode = "windowed"
        end

        -- Apply fullscreen/windowed mode (PC only)
        if setFullscreen then
            setFullscreen(_G.GraphicsSettings.displayMode == "fullscreen")
        end

        -- Save to settings file (disabled: fullscreen should not persist across launches)
        -- if settingsSave then
        --     settingsSave("gfx_displaymode", _G.GraphicsSettings.displayMode)
        -- end

        updateGraphicsModeDisplay()
        printLog("[UI] graphics_Right -> " .. tostring(_G.GraphicsSettings.displayMode))
    end,

    ----------------------------------------------------------------------
    -- RESET GRAPHICS SETTINGS
    ----------------------------------------------------------------------
    reset_Graphics_Settings = function(buttonEntity, payload)
        playUIClick()

        -- Slider positioning constants (must match UISlider.lua)
        local sliderMinX = -0.32
        local sliderMaxX = 0.32

        -- Default slider values (0.0-1.0 range)
        local DEFAULT_BRIGHTNESS = 0.333   -- maps to exposure 1.0 (range 0.5-2.0)
        local DEFAULT_GAMMA      = 0.467   -- maps to gamma 2.2 (range 1.5-3.0)

        -- 1. Reset display mode to windowed
        _G.GraphicsSettings.displayMode = "windowed"
        if setFullscreen then
            setFullscreen(false)
        end
        updateGraphicsModeDisplay()

        -- 2. Reset brightness (exposure = 0.5 + 0.333 * 1.5 = 1.0)
        local defaultExposure = 0.5 + DEFAULT_BRIGHTNESS * 1.5
        if setBrightness then
            setBrightness(defaultExposure)
        end
        _G.SliderUI = _G.SliderUI or {}
        _G.SliderUI["brightness_handle"] = DEFAULT_BRIGHTNESS

        local bHandle = findEntity("brightness_handle")
        if bHandle then
            local _, knobY = get2DPosition(bHandle)
            local newX = sliderMinX + DEFAULT_BRIGHTNESS * (sliderMaxX - sliderMinX)
            set2DPosition(bHandle, newX, knobY)
        end

        if settingsSave then
            settingsSave("gfx_brightness", string.format("%.4f", DEFAULT_BRIGHTNESS))
        end

        -- 3. Reset gamma (gamma = 1.5 + 0.467 * 1.5 = 2.2)
        local defaultGamma = 1.5 + DEFAULT_GAMMA * 1.5
        if setGamma then
            setGamma(defaultGamma)
        end
        _G.SliderUI["gamma_handle"] = DEFAULT_GAMMA

        local gHandle = findEntity("gamma_handle")
        if gHandle then
            local _, knobY = get2DPosition(gHandle)
            local newX = sliderMinX + DEFAULT_GAMMA * (sliderMaxX - sliderMinX)
            set2DPosition(gHandle, newX, knobY)
        end

        if settingsSave then
            settingsSave("gfx_gamma", string.format("%.4f", DEFAULT_GAMMA))
        end

        printLog("[UI] reset_Graphics_Settings -> windowed, brightness=" .. defaultExposure .. ", gamma=" .. defaultGamma)
    end,

    ----------------------------------------------------------------------
    -- SETTINGS MENU
    ----------------------------------------------------------------------
    -- Goes from main menu to settings
    mainmenu_Next = function(buttonEntity, payload)
        playUIClick()

        -- Disable main menu layer
        setLayerEnabled(layers.MAINMENU, false)

        -- Enable next layer
        if payload then
            local layer = layers[string.upper(payload)]

            if layer then
                setLayerEnabled(layer, true)
            else
                printLog("[UI] Invalid payload: " .. tostring(payload))
            end
        else
            printLog("[UI] No payload provided")
        end

        -- setLayerEnabled(layers.SETTINGS, true)
    end,

    -- Goes back from settings to main menu
    settings_Back = function(buttonEntity, payload)
        playUIClick()

        -- Disable settings layers
        setLayerEnabled(layers.AUDIO, false)
        setLayerEnabled(layers.GRAPHICS, false)
        setLayerEnabled(layers.CONTROLS, false)
        setLayerEnabled(layers.SETTINGS, false)
        
        -- Enable main menu layers
        setLayerEnabled(layers.MAINMENU, true)
    end,

    -- Goes back from audio/graphics/controls to settings
    settings_Sub_Back = function(buttonEntity, payload)
        playUIClick()

        -- Disable other settings ui layers
        setLayerEnabled(layers.AUDIO, false)
        setLayerEnabled(layers.GRAPHICS, false)
        setLayerEnabled(layers.CONTROLS, false)
        
        -- Enable settingsUI layer
        setLayerEnabled(layers.SETTINGS, true)
    end,

    -- Goes from settings to another layer. Payload: layer to show
    settings_Next = function(buttonEntity, payload)
        playUIClick()

        -- Block controls rebinding menu on Android
        if payload and string.upper(payload) == "CONTROLS" then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                printLog("[UI] Controls rebinding not available on Android")
                return
            end
        end

        -- Disable settingsUI layer
        setLayerEnabled(layers.SETTINGS, false)

        if payload then
            local layer = layers[string.upper(payload)]

            if layer then
                setLayerEnabled(layer, true)
            else
                printLog("[UI] Invalid payload: " .. tostring(payload))
                setLayerEnabled(layers.SETTINGS, true)
            end
        else
            printLog("[UI] No payload provided")
            setLayerEnabled(layers.SETTINGS, true)
        end
    end,

    -- Goes from a setting layer to main menu, Payload: Layer to hide
    to_MainMenu = function(buttonEntity, payload)
        playUIClick()

        if payload then
            local layer = layers[string.upper(payload)]

            if layer then
                -- Disable previous layer
                setLayerEnabled(layer, false)
            else
                printLog("[UI] Invalid payload: " .. tostring(payload))
                return
            end
        else
            printLog("[UI] No payload provided")
            return
        end
        
        -- Enable main menu layer
        setLayerEnabled(layers.MAINMENU, true)
    end,

    -- Toggles between credit layers
    credits_Next = function(buttonEntity, payload)
        playUIClick()

        -- Disable previous layer
        if payload then
            local layer = layers[string.upper(payload)]

            if layer == layers.CREDITS_1 then
                setLayerEnabled(layers.CREDITS_1, true)
                setLayerEnabled(layers.CREDITS_2, false)
            elseif layer == layers.CREDITS_2 then
                setLayerEnabled(layers.CREDITS_2, true)
                setLayerEnabled(layers.CREDITS_1, false)
            end
        else
            printLog("[UI] No payload provided")
            return
        end
    end,

    -- Toggles between how to play layers
    howtoplay_Next = function(buttonEntity, payload)
        playUIClick()

        -- Disable previous layer
        if payload then
            local layer = layers[string.upper(payload)]

            if layer == layers.HOWTOPLAY_1 then
                setLayerEnabled(layers.HOWTOPLAY_1, true)
                setLayerEnabled(layers.HOWTOPLAY_2, false)
            elseif layer == layers.HOWTOPLAY_2 then
                setLayerEnabled(layers.HOWTOPLAY_2, true)
                setLayerEnabled(layers.HOWTOPLAY_1, false)
            end
        else
            printLog("[UI] No payload provided")
            return
        end
    end,

    ----------------------------------------------------------------------
    -- CONTROLS REBINDING
    ----------------------------------------------------------------------
    rebind_Forward = function(buttonEntity, payload)
        playUIClick()
        local KB = _G.KeyBindings
        if KB then
            KB.startRebind("forward", buttonEntity)
            printLog("[UI] Rebinding forward... press a key")
        end
    end,

    rebind_Left = function(buttonEntity, payload)
        playUIClick()
        local KB = _G.KeyBindings
        if KB then
            KB.startRebind("left", buttonEntity)
            printLog("[UI] Rebinding left... press a key")
        end
    end,

    rebind_Right = function(buttonEntity, payload)
        playUIClick()
        local KB = _G.KeyBindings
        if KB then
            KB.startRebind("right", buttonEntity)
            printLog("[UI] Rebinding right... press a key")
        end
    end,

    rebind_Backward = function(buttonEntity, payload)
        playUIClick()
        local KB = _G.KeyBindings
        if KB then
            KB.startRebind("backward", buttonEntity)
            printLog("[UI] Rebinding backward... press a key")
        end
    end,

    rebind_Jump = function(buttonEntity, payload)
        playUIClick()
        local KB = _G.KeyBindings
        if KB then
            KB.startRebind("jump", buttonEntity)
            printLog("[UI] Rebinding jump... press a key")
        end
    end,

    rebind_Hide = function(buttonEntity, payload)
        playUIClick()
        local KB = _G.KeyBindings
        if KB then
            KB.startRebind("hide", buttonEntity)
            printLog("[UI] Rebinding hide... press a key")
        end
    end,

    reset_Controls = function(buttonEntity, payload)
        playUIClick()
        local KB = _G.KeyBindings
        if KB then
            KB.cancelRebind()
            KB.resetAll()
            printLog("[UI] Controls reset to defaults")

            -- Update all UI text entities for the 6 controls
            -- The UI entities should be named: "bind_forward", "bind_left", etc.
            local actionNames = { "forward", "left", "right", "backward", "jump", "hide" }
            for _, action in ipairs(actionNames) do
                local entityName = "bind_" .. action
                local e = findEntity(entityName)
                if e and setUIText then
                    local keyName = KB[action] or ""
                    local displayName = KB.getDisplayName and KB.getDisplayName(keyName) or keyName
                    setUIText(e, displayName)
                    printLog("[UI] Reset " .. action .. " display to " .. displayName)
                end
            end
        end
    end,
}

function G.UI_OnAction(actionName, buttonEntity, payload)
    local h = handlers[actionName]
    if h then
        -- Play UI click sound for all button presses
        h(buttonEntity, payload)
    else
        printLog("[UI] No handler for action "..tostring(actionName))
    end
end

-- ==================== KEY CAPTURE FOR CONTROLS REBINDING ====================
-- Register key handlers for all rebindable keys to capture rebind input.
-- These only do work when _G.KeyBindings._pendingRebind is set (event-driven, not per-frame).
local REBIND_LISTEN_KEYS = {
    "A","B","C","D","E","F","G","H","I","J","K","L","M",
    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    "0","1","2","3","4","5","6","7","8","9",
    "SPACE","TAB","LSHIFT","RSHIFT"
}

for _, keyName in ipairs(REBIND_LISTEN_KEYS) do
    registerKeyDown(keyName, function()
        local KB = _G.KeyBindings
        if KB and KB._pendingRebind then
            KB.tryApplyRebind(keyName)
        end
    end)
end

-- ==================== INIT: Sync display text on scene load ====================
-- Wait a few frames so GlobalAudioController has time to load saved settings
local uiactions_initDone = false
local uiactions_frameCount = 0
local UIACTIONS_FRAMES_TO_WAIT = 5  -- GlobalAudioController waits 3, so we wait 5

registerUpdate(function(dt)
    if uiactions_initDone then return end
    uiactions_frameCount = uiactions_frameCount + 1
    if uiactions_frameCount >= UIACTIONS_FRAMES_TO_WAIT then
        uiactions_initDone = true
        updateGraphicsModeDisplay()

        -- Sync controls keybinding display text
        local KB = _G.KeyBindings
        if KB then
            local actionNames = { "forward", "left", "right", "backward", "jump", "hide" }
            for _, action in ipairs(actionNames) do
                local entityName = "bind_" .. action
                local e = findEntity(entityName)
                if e and setUIText then
                    local keyName = KB[action] or ""
                    local displayName = KB.getDisplayName and KB.getDisplayName(keyName) or keyName
                    setUIText(e, displayName)
                end
            end
        end
    end
end)
