
-- next scene script
local sceneChange = false
local nextScene = nil

-- for UI button (on_click_callback_lua = "StartButton_OnClick")
_G_root.StartButton_OnClick = function()
    sceneChange = true
    nextScene = "prototype.scn"
end

-- for UI button (on_click_callback_lua = "HowToPlayButton_OnClick")
_G_root.HowToPlayButton_OnClick = function()
    sceneChange = true
    nextScene = "howtoplay.scn"
end

-- for UI button (on_click_callback_lua = "SettingsButton_OnClick")
_G_root.SettingsButton_OnClick = function()
    sceneChange = true
    --nextScene = "settings.scn"
end

-- for UI button (on_click_callback_lua = "CreditsButton_OnClick")
_G_root.CreditsButton_OnClick = function()
    sceneChange = true
    --nextScene = "credits.scn"
end

-- for UI button (on_click_callback_lua = "QuitButton_OnClick")
_G_root.QuitButton_OnClick = function()
    --quit game
end

-- for UI button (on_click_callback_lua = "QuitToStartButton_OnClick")
_G_root.QuitToStartButton_OnClick = function()
    sceneChange = true
    nextScene = "mainmenu.scn"
end

-- for UI button (on_click_callback_lua = "BackButton_OnClick")
_G_root.BackButton_OnClick = function()
    sceneChange = true
    nextScene = "mainmenu.scn"
end

registerUpdate(function(dt)
    if sceneChange then

        if nextScene == nil then
            return
        end

        changeScene(nextScene)
        sceneChange = false
        nextScene = nil
    end
end)