
-- next scene script
local sceneChange = false

-- for UI button (on_click_callback_lua = "StartButton_OnClick")
_G_root.StartButton_OnClick = function()
    printLog("[UI] Starting button pressed, setting sceneChange")
    sceneChange = true
end

registerUpdate(function(dt)
    if sceneChange then
        sceneChange = false
    end

    --changeScene()

end)