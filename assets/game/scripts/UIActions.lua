-- UIActions.lua

local G = _G_root

-- scene defaults
G.CurrentLevelName    = G.CurrentLevelName   or "Tutorial.scn"
G.FirstLevelScene     = G.FirstLevelScene    or "Tutorial.scn"
G.TutorialSceneName   = G.TutorialSceneName  or "Tutorial.scn"

G.MainMenuSceneName   = G.MainMenuSceneName  or "mainmenu.scn"    -- main menu scene
G.HowToPlaySceneName  = G.HowToPlaySceneName or "howtoplay.scn"   -- how to play scene
G.HowToPlaySceneName2  = G.HowToPlaySceneName2 or "howtoplay2.scn"   -- how to play scene 2
G.SettingsSceneName   = G.SettingsSceneName  or "settings.scn"   -- how to play scene
G.CreditsSceneName    = G.CreditsSceneName   or "credits.scn"   -- how to play scene

local function resolveSceneName(payload, fallback)
    if payload == nil or payload == "" then
        return fallback
    end
    return payload
end

local PlayerState = _G.PlayerState

local handlers = {
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
        if _G_root.gamePaused then
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
    pause_Resume = function(buttonEntity, payload)
        if _G_root.TogglePause then
            _G_root.TogglePause()
            printLog("[UI] pause_Resume -> called TogglePause()")
        else
            printLog("[UI] pause_Resume pressed, but TogglePause is not available")
        end
    end,

    pause_Restart = function(buttonEntity, payload)
        -- Hardcoded to always restart Tutorial.scn
        printLog("[UI] pause_Restart -> restarting Tutorial.scn")
        
        if _G_root.gamePaused then
            _G_root.gamePaused = false
            printLog("[UI] Game unpaused before restart")
        end
        
        if changeScene then
            changeScene("Tutorial.scn")
        else
            printLog("[UI] pause_Restart pressed, but changeScene is not bound")
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
            setLayerEnabled(4, true)  -- Show QuitOverlay layer (layer 4)
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
        if _G_root.gamePaused then
            _G_root.gamePaused = false
            printLog("[UI] Game unpaused before scene change")
        end
        
        -- Hide quit overlay
        if setLayerEnabled then
            setLayerEnabled(4, false)
            printLog("[UI] QuitOverlay hidden")
        end
        
        -- Load main menu - HARDCODED
        printLog("[UI] quit_Confirm -> changeScene(mainmenu.scn)")
        if changeScene then
            changeScene("mainmenu.scn")
        else
            printLog("[UI] quit_Confirm pressed, but changeScene is not bound")
        end
    end,

    quit_Cancel = function(buttonEntity, payload)
        -- User pressed NO - close popup
        printLog("[UI] quit_Cancel -> closing quit confirmation")
        
        if setLayerEnabled then
            setLayerEnabled(4, false)
            printLog("[UI] QuitOverlay (layer 4) hidden - returning to pause menu")
        else
            printLog("[UI] setLayerEnabled not available")
        end
    end,

    ----------------------------------------------------------------------
    -- MAIN MENU
    ----------------------------------------------------------------------
    menu_StartGame = function(buttonEntity, payload)
        printLog("[UI] menu_StartGame -> changeScene(Tutorial.scn)")
        if changeScene then
            changeScene("Tutorial.scn")
        else
            printLog("[UI] menu_StartGame pressed, but changeScene is not bound")
        end
    end,

    menu_OpenSettings = function(buttonEntity, payload)
        local settingsScene = resolveSceneName(payload, G.SettingsSceneName, "settings.scn")

        if settingsScene and changeScene then
            printLog("[UI] menu_OpenSettings -> changeScene("..settingsScene..")")
            changeScene(settingsScene)
        else
            printLog("[UI] menu_OpenSettings pressed (no scene specified / not implemented)")
        end
    end,

    menu_HowToPlay = function(buttonEntity, payload)
        local howtoplayScene = resolveSceneName(payload or G.HowToPlaySceneName, "howtoplay.scn")

        if howtoplayScene and changeScene then
            printLog("[UI] menu_HowToPlay -> changeScene("..howtoplayScene..")")
            changeScene(howtoplayScene)
        else
            printLog("[UI] menu_HowToPlay pressed (no scene specified / not implemented)")
        end
    end,

    menu_Credits = function(buttonEntity, payload)
        local creditsScene = resolveSceneName(payload or G.CreditsSceneName, "credits.scn")

        if creditsScene and changeScene then
            printLog("[UI] menu_Credits -> changeScene("..creditsScene..")")
            changeScene(creditsScene)
        else
            printLog("[UI] menu_Credits pressed (no scene specified / not implemented)")
        end
    end,

    menu_QuitGame = function(buttonEntity, payload)
        if quitApplication then
            printLog("[UI] menu_QuitGame -> quitApplication()")
            quitApplication()
        else
            printLog("[UI] menu_QuitGame pressed (quitapplication() not bound; implement in EngineAPI)")
        end
    end,

    menu_OpenTutorial = function(buttonEntity, payload)
        local tutorialScene = resolveSceneName(payload or G.TutorialSceneName, "Tutorial.scn")

        if tutorialScene and changeScene then
            printLog("[UI] menu_OpenTutorial -> changeScene("..tutorialScene..")")
            changeScene(tutorialScene)
        else
            printLog("[UI] menu_OpenTutorial pressed (no scene specified / not implemented)")
        end
    end,

    menu_BackToMain = function(buttonEntity, payload)
        printLog("[UI] menu_BackToMain -> changeScene(mainmenu.scn)")
        if changeScene then
            changeScene("mainmenu.scn")
        else
            printLog("[UI] menu_BackToMain pressed (no scene specified / not implemented)")
        end
    end,


    ----------------------------------------------------------------------
    -- HOW TO PLAY
    ----------------------------------------------------------------------
    howtoplay_ArrowLeft = function(buttonEntity, payload)
        -- Back to How To Play Page 1
        local howtoplayScene = resolveSceneName(payload or G.HowToPlaySceneName, "howtoplay.scn")

        if howtoplayScene and changeScene then
            printLog("[UI] backtohowToPlay1 -> changeScene("..howtoplayScene..")")
            changeScene(howtoplayScene)
        else
            printLog("[UI] backtohowToPlay1 pressed (no scene specified / not implemented)")
        end
    end,

    howtoplay_ArrowRight = function(buttonEntity, payload)
        -- Back to How To Play Page 2
        local howtoplayScene2 = resolveSceneName(payload or G.HowToPlaySceneName2, "howtoplay2.scn")

        if howtoplayScene2 and changeScene then
            printLog("[UI] backtohowToPlay2 -> changeScene("..howtoplayScene2..")")
            changeScene(howtoplayScene2)
        else
            printLog("[UI] backtohowToPlay2 pressed (no scene specified / not implemented)")
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
