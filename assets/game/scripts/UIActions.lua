-- UIActions.lua

local G = _G_root

-- scene defaults
G.CurrentLevelName    = G.CurrentLevelName   or "prototype.scn"   -- current gameplay level
G.FirstLevelScene     = G.FirstLevelScene    or "prototype.scn"   -- first level from main menu
G.TutorialSceneName   = G.TutorialSceneName  or "Tutorial.scn"   -- tutorial scene

G.MainMenuSceneName   = G.MainMenuSceneName  or "mainmenu.scn"    -- main menu scene
G.HowToPlaySceneName  = G.HowToPlaySceneName or "howtoplay.scn"   -- how to play scene
G.HowToPlaySceneName2  = G.HowToPlaySceneName2 or "howtoplay2.scn"   -- how to play scene 2
G.SettingsSceneName   = G.SettingsSceneName  or "settings.scn"   -- how to play scene
G.CreditsSceneName    = G.CreditsSceneName   or "credits.scn"   -- how to play scene

G.SettingsSceneName   = G.SettingsSceneName  or "pausemenu.scn"   -- pause settings

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
        -- PlayerInput API from playerMovement.lua
        if _G.PlayerInput and _G.PlayerInput.requestJump then
            _G.PlayerInput.requestJump()
            return
        end

        -- fallback 
        if PlayerState and PlayerState.isHidden and PlayerState.isHidden() then
            return
        end

        printLog("[UI] game_Jump pressed, but PlayerInput.requestJump is missing")
    end,

    game_Move = function(buttonEntity, payload)
        -- Joystick movement callback
        -- Payload should be a table or string containing dirX and dirY
        -- This is called by the UI system when joystick is dragged
        
        if _G_root.gamePaused then
            -- When paused, send zero movement
            if _G_root.Joystick_OnDrag then
                _G_root.Joystick_OnDrag(0.0, 0.0)
            end
            return
        end
        
        -- Parse payload if it's a string (format: "dirX,dirY")
        local dirX, dirY = 0.0, 0.0
        
        if type(payload) == "string" then
            -- Parse "x,y" format
            local values = {}
            for val in string.gmatch(payload, "[^,]+") do
                table.insert(values, tonumber(val) or 0.0)
            end
            dirX = values[1] or 0.0
            dirY = values[2] or 0.0
        elseif type(payload) == "table" then
            -- If payload is a table with x,y or dirX,dirY
            dirX = payload.dirX or payload.x or 0.0
            dirY = payload.dirY or payload.y or 0.0
        end
        
        -- Call the existing joystick callback from playerMovement.lua
        if _G_root.Joystick_OnDrag then
            _G_root.Joystick_OnDrag(dirX, dirY)
        else
            printLog("[UI] game_Move pressed, but Joystick_OnDrag is missing")
        end
    end,


    game_Hide = function(buttonEntity, payload)
        -- hide/unhide button
        if PlayerState and PlayerState.onHideButton then
            PlayerState.onHideButton()
        else
            printLog("[UI] game_Hide pressed, but PlayerState.onHideButton is missing")
        end
    end,

    game_Collect = function(buttonEntity, payload)
        -- collect/deliver button
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
        -- Simply unpause the game
        if pauseAllSystems then
            pauseAllSystems(false)
        else
            printLog("[UI] pause_Resume pressed, but pauseAllSystems is not bound")
        end
    end,

    pause_Restart = function(buttonEntity, payload)
        -- reload current level
        -- priority: button payload > global CurrentLevelName > hardcoded default
        local levelName = resolveSceneName(payload, G.CurrentLevelName,"Tutorial.scn")

        if changeScene then
            printLog("[UI] pause_Restart -> changeScene("..levelName..")")
            changeScene(levelName)
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
        local mainMenu = resolveSceneName(payloa, G.MainMenuSceneName, "mainmenu.scn")

        if changeScene then
            printLog("[UI] pause_ReturnToMainMenu -> changeScene("..mainMenu..")")
            changeScene(mainMenu)
        else
            printLog("[UI] pause_ReturnToMainMenu pressed, but changeScene is not bound")
        end
    end,

    ----------------------------------------------------------------------
    -- MAIN MENU
    ----------------------------------------------------------------------
    menu_StartGame = function(buttonEntity, payload)
        -- Start first level
        local levelName = resolveSceneName(payload, G.FirstLevelScene, "Tutorial.scn")

        if changeScene then
            printLog("[UI] menu_StartGame -> changeScene("..levelName..")")
            changeScene(levelName)
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
            printLog("[UI] menu_OpenSettings -> changeScene("..howtoplayScene..")")
            changeScene(howtoplayScene)
        else
            printLog("[UI] menu_OpenSettings pressed (no scene specified / not implemented)")
        end
    end,

    menu_Credits = function(buttonEntity, payload)
        local creditsScene = resolveSceneName(payload or G.CreditsSceneName, "credits.scn")

        if creditsScene and changeScene then
            printLog("[UI] menu_OpenSettings -> changeScene("..creditsScene..")")
            changeScene(creditsScene)
        else
            printLog("[UI] menu_OpenSettings pressed (no scene specified / not implemented)")
        end
    end,

    menu_QuitGame = function(buttonEntity, payload)
        if quitApplication then
            printLog("[UI] menu_QuitGame -> quitApplication()")
            quitApplication() -- i think no such thing yet
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
        local backtomainScene = resolveSceneName(payload or G.TutorialSceneName, "mainmenu.scn")

        if backtomainScene and changeScene then
            printLog("[UI] menu_OpenTutorial -> changeScene("..backtomainScene..")")
            changeScene(backtomainScene)
        else
            printLog("[UI] menu_OpenTutorial pressed (no scene specified / not implemented)")
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
