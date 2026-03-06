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
    -- HOW TO PLAY
    ----------------------------------------------------------------------


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
                                 string.find(sceneName, "cutscene")
            
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
    -- CREDITS
    ----------------------------------------------------------------------
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
