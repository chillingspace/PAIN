-- UIActions.lua

local G = _G_root

-- scene defaults
G.CurrentLevelName    = G.CurrentLevelName   or "prototype.scn"   -- current gameplay level
G.FirstLevelScene     = G.FirstLevelScene    or "prototype.scn"   -- first level from main menu
G.TutorialSceneName   = G.TutorialSceneName  or "Tutorial.scn"   -- tutorial scene

G.MainMenuSceneName   = G.MainMenuSceneName  or "mainmenu.scn"    -- main menu scene
G.HowToPlaySceneName  = G.HowToPlaySceneName or "howtoplay.scn"   -- how to play scene
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
        local levelName = resolveSceneName(payload, G.CurrentLevelName,"prototype.scn")

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
        local levelName = resolveSceneName(payload, G.FirstLevelScene, "prototype.scn")

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

}

function G.UI_OnAction(actionName, buttonEntity, payload)
    local h = handlers[actionName]
    if h then
        h(buttonEntity, payload)
    else
        printLog("[UI] No handler for action "..tostring(actionName))
    end
end
