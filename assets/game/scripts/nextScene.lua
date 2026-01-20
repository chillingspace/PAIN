
-- next scene script
local sceneChange = false
local nextScene = nil

-- for UI button (on_click_callback_lua = "Button_OnClick")
_G_root.Button_OnClick = function()
    sceneChange = true
end

registerUpdate(function(dt)
    if sceneChange then

        -- Main Menu Buttons
        if getEntityName(entityId) == "start_button" then
            printLog("[UI] Start button pressed")
            nextScene = "prototype.scn"
        end
        if getEntityName(entityId) == "how_to_play_button" then
            printLog("[UI] How To Play button pressed")
            nextScene = "howtoplay.scn"
        end
        if getEntityName(entityId) == "settings_button" then
            printLog("[UI] Settings button pressed")
            --nextScene = "settings.scn"
        end
        if getEntityName(entityId) == "credits_button" then
            printLog("[UI] Credits button pressed")
            --nextScene = "credits.scn"
        end
        if getEntityName(entityId) == "quit_button" then
            printLog("[UI] Quit button pressed")
            --exit app
        end


        -- Back Button
        if getEntityName(entityId) == "back_button" then
            printLog("[UI] Start button pressed")
            nextScene = "mainmenu.scn"
        end


        if nextScene == nil then
            return
        end

        changeScene(nextScene)
        sceneChange = false
    end
end)