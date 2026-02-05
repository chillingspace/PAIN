-- UIActions.lua

local G = _G_root

-- scene defaults
G.CurrentLevelName    = G.CurrentLevelName   or "game/scenes/Tutorial.scn"
G.FirstLevelScene     = G.FirstLevelScene    or "game/scenes/Level1.scn"
G.TutorialSceneName   = G.TutorialSceneName  or "game/scenes/Tutorial.scn"
G.MainMenuSceneName   = G.MainMenuSceneName  or "game/scenes/mainmenu.scn"
G.HowToPlaySceneName  = G.HowToPlaySceneName or "game/scenes/howtoplay.scn"
G.HowToPlaySceneName2 = G.HowToPlaySceneName2 or "game/scenes/howtoplay2.scn"
G.CreditsSceneName    = G.CreditsSceneName   or "game/scenes/credits.scn"

-- Placeholder for next level
-- G.NextLevelName       = G.NextLevelName      or "game/scenes/Tutorial.scn"

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

-- Store current level name globally (For Game_PlayerState)
if G and G.CurrentLevelName then
    _G.CurrentLevelName = G.CurrentLevelName
end

if G and G.NextLevelName then
    _G.NextLevelName = G.NextLevelName
end

-- print("[UIActions] running, _G_root=", _G_root)
-- print("[UIActions] before default, CurrentLevelName=", _G_root and _G_root.CurrentLevelName)

local function resolveSceneName(payload, fallback)
    if payload == nil or payload == "" then
        return fallback
    end
    return payload
end

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

    end_Restart = function()
        if changeScene then
            changeScene(G.CurrentLevelName)
        end
    end,

    end_MainMenu = function()
        if changeScene then
            changeScene(G.MainMenuSceneName)
        end
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
            --SetGamePaused(not IsGamePaused()) 
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
        
        if setLayerEnabled then
            setLayerEnabled(Layers.RESTART, true)  -- Show RestartOverlay layer (layer 5)
            setLayerEnabled(Layers.PAUSE, false) -- Hide PauseMenu
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            printLog("[UI] RestartOverlay (layer 5) shown")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,

    restart_Confirm = function(buttonEntity, payload)
        -- User pressed YES - restart level
        printLog("[UI] restart_Confirm -> restarting")
        
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
        
        -- Restart Scene
        if setCurrentScene then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
            end
            
            local curr = ""
            if getCurrentSceneName then curr = getCurrentSceneName() end
            if curr == "" then curr = G.CurrentLevelName end
            
            printLog("[UI] restart_Confirm -> setCurrentScene("..tostring(curr)..")")
            setCurrentScene(curr)
        elseif changeScene then
            changeScene(G.CurrentLevelName)
        else
            printLog("[UI] restart_Confirm pressed, but setCurrentScene is not bound")
        end
    end,

    restart_Cancel = function(buttonEntity, payload)
        -- User pressed NO - close popup
        printLog("[UI] restart_Cancel -> closing restart confirmation")
        
        if setLayerEnabled then
            setLayerEnabled(Layers.RESTART, false)
            setLayerEnabled(Layers.PAUSE, true) -- Return to Pause Menu
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
            end
            printLog("[UI] Overlay (layer 5) hidden - returning to pause menu")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,

    pause_Settings = function(buttonEntity, payload)
        local settingsScene = resolveSceneName(payload, G.SettingsSceneName, "settings.scn")

        if settingsScene and changeScene then
            printLog("[UI] pause_Settings -> changeScene("..settingsScene..")")
            changeScene(settingsScene)
        else
            printLog("[UI] pause_Settings pressed (no scene specified / not implemented)")
        end
    end,

    pause_ReturnToMainMenu = function(buttonEntity, payload)
        -- Show confirmation popup
        printLog("[UI] pause_ReturnToMainMenu -> showing quit confirmation")
        
        if setLayerEnabled then
            setLayerEnabled(Layers.QUIT, true)  -- Show QuitOverlay layer (layer 4)
            setLayerEnabled(Layers.PAUSE, false)
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
                
            end
            printLog("[UI] QuitOverlay (layer 4) shown")
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
        
        -- Load main menu 
        printLog("[UI] quit_Confirm -> changeScene(" .. G.MainMenuSceneName .. ")")
        if changeScene then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
                
            end
            changeScene(G.MainMenuSceneName)
        else
            printLog("[UI] quit_Confirm pressed, but changeScene is not bound")
        end
    end,

    quit_Cancel = function(buttonEntity, payload)
        -- User pressed NO - close popup
        printLog("[UI] quit_Cancel -> closing quit confirmation")
        
        if setLayerEnabled then
            setLayerEnabled(Layers.QUIT, false)
            setLayerEnabled(Layers.PAUSE, true)
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
    menu_StartGame = function(buttonEntity, payload)

        --local firstLevelScene = resolveSceneName(payload, G.FirstLevelScene, "Level1.scn")

        --printLog("[UI] menu_StartGame -> changeScene(" .. firstLevelScene .. ")")

        -- Reset camera state before scene transition
        -- if _G.ResetThirdPersonCamera then
        --     _G.ResetThirdPersonCamera()
        -- end

        if G.TutorialSceneName and setCurrentScene then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
            end
            changeSceneWithAudioFade(G.TutorialSceneName)
        else
            printLog("[UI] menu_StartGame pressed, but setCurrentScene is not bound")
        end
    end,

    menu_HowToPlay = function(buttonEntity, payload)
        --local howtoplayScene = resolveSceneName(payload or G.HowToPlaySceneName, "howtoplay.scn")

        if G.HowToPlaySceneName and setCurrentScene then
            printLog("[UI] menu_HowToPlay -> setCurrentScene("..G.HowToPlaySceneName..")")
            changeSceneWithAudioFade(G.HowToPlaySceneName)
        else
            printLog("[UI] menu_HowToPlay pressed (no scene specified / not implemented)")
        end
    end,

    menu_Credits = function(buttonEntity, payload)
        --local creditsScene = resolveSceneName(payload or G.CreditsSceneName, "credits.scn")

        --if G.CreditsSceneName and setCurrentScene then
        --    printLog("[UI] menu_Credits -> setCurrentScene("..G.CreditsSceneName..")")
        --    setCurrentScene(G.CreditsSceneName)
        --else
            printLog("[UI] menu_Credits pressed (no scene specified / not implemented)")
        --end
    end,

    menu_QuitGame = function(buttonEntity, payload)
        if IsGamePaused() then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                Hide_Cursor(true)
            end
            --SetGamePaused(not IsGamePaused()) 
            _G_root.TogglePause()
            
            setLayerEnabled(2, true)
            printLog("[UI] cutscene_Menu_Button -> called TogglePause()")
        else
            printLog("[UI] cutscene_Menu_Button pressed, but TogglePause is not available")
        end
    end,

    menu_OpenTutorial = function(buttonEntity, payload)
        --local tutorialScene = resolveSceneName(payload or G.TutorialSceneName, "Tutorial.scn")

        if G.TutorialSceneName and setCurrentScene then
            printLog("[UI] menu_OpenTutorial -> setCurrentScene("..G.TutorialSceneName..")")
            setCurrentScene(G.TutorialSceneName)
        else
            printLog("[UI] menu_OpenTutorial pressed (no scene specified / not implemented)")
        end
    end,

    menu_BackToMain = function(buttonEntity, payload)
        -- User requested to always go back to Main Menu (avoids loop in HowToPlay scenes)
        local target = G.MainMenuSceneName
        
        printLog("[UI] menu_BackToMain -> setCurrentScene("..tostring(target)..")")
        if setCurrentScene then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
                
            end
            setCurrentScene(target)
        else
            printLog("[UI] menu_BackToMain pressed (setCurrentScene not bound)")
        end
        changeSceneWithAudioFade(G.MainMenuSceneName)
    end,


    ----------------------------------------------------------------------
    -- HOW TO PLAY
    ----------------------------------------------------------------------
    howtoplay_ArrowLeft = function(buttonEntity, payload)
        -- Navigate to How To Play Page 1 (no fade - same audio context)
        if G.HowToPlaySceneName and changeScene then
            printLog("[UI] howtoplay_ArrowLeft -> " .. G.HowToPlaySceneName)
            changeScene(G.HowToPlaySceneName)  -- Direct change, same global audio continues
        else
            printLog("[UI] howtoplay_ArrowLeft: scene not set")
        end
    end,

    howtoplay_ArrowRight = function(buttonEntity, payload)
        -- Navigate to How To Play Page 2 (no fade - same audio context)
        if G.HowToPlaySceneName2 and changeScene then
            printLog("[UI] howtoplay_ArrowRight -> " .. G.HowToPlaySceneName2)
            changeScene(G.HowToPlaySceneName2)  -- Direct change, same global audio continues
        else
            printLog("[UI] howtoplay_ArrowRight: scene not set")
        end
    end,


    ----------------------------------------------------------------------
    -- CUT SCENE
    ----------------------------------------------------------------------
    cutscene_Open_Menu = function(buttonEntity, payload)
        -- Android touch button callback to pause the game
        if _G_root.TogglePause then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(false)
                
            end
            _G_root.TogglePause()
            printLog("[UI] cutscene_Open_Menu (Android) -> called TogglePause()")
        else
            printLog("[UI] cutscene_Open_Menu pressed, but TogglePause is not available")
        end
    end,

    cutscene_Close_Menu = function(buttonEntity, payload)
        if IsGamePaused() then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
            end
            --SetGamePaused(not IsGamePaused()) 
            _G_root.TogglePause()
            
            setLayerEnabled(1, not _G_root.IsGamePaused())
            setLayerEnabled(2, _G_root.IsGamePaused())
            printLog("[UI] cutscene_Close_Menu -> called TogglePause()")
        else
            printLog("[UI] cutscene_Close_Menu pressed, but TogglePause is not available")
        end
    end,

    cutscene_Menu_Quit = function(buttonEntity, payload)
        if IsGamePaused() then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
            end
            --SetGamePaused(not IsGamePaused()) 
            _G_root.TogglePause()
            
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
            SetGamePaused(false) 
            printLog("[UI] Game unpaused before scene change")
        end
        
        -- Hide quit overlay
        if setLayerEnabled then
            setLayerEnabled(Layers.QUIT, false)
            printLog("[UI] QuitOverlay hidden")
        end
        
        -- Load main menu 
        printLog("[UI] cutscene_Quit_Confirm -> changeScene(" .. G.MainMenuSceneName .. ")")
        if changeScene then
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                hideCursor(true)
                
            end
            changeScene(G.MainMenuSceneName)
        else
            printLog("[UI] cutscene_Quit_Confirm pressed, but changeScene is not bound")
        end
    end,

    cutscene_Quit_Cancel = function(buttonEntity, payload)
        -- User pressed NO - close popup
        printLog("[UI] cutscene_Quit_Cancel -> closing quit confirmation")
        
        if setLayerEnabled then
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

            setLayerEnabled(2, false) -- Return to Menu
            local isMobile = (isAndroid ~= nil and isAndroid())
            if isMobile then
                Hide_Cursor(false)
            end
            --_G_root.TogglePause()
            printLog("[UI] QuitOverlay (layer 5) hidden - returning to menu")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,
}

function G.UI_OnAction(actionName, buttonEntity, payload)
    local h = handlers[actionName]
    if h then
        h(buttonEntity, payload)
    else
        printLog("[UI] No handler for action "..tostring(actionName))
    end
end
