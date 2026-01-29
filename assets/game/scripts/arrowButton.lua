
-- arrow button clicked script
local arrowClicked = false

-- for UI button (on_click_callback_lua = "RightArrowButton_OnClick")
_G_root.RightArrowButton_OnClick = function()
    arrowClicked = true
end

-- for UI button (on_click_callback_lua = "LeftArrowButton_OnClick")
_G_root.LeftArrowButton_OnClick = function()
    arrowClicked = true
end

registerUpdate(function(dt)

end)