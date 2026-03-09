-- UISlider.lua
-- Attach to slider handle entity

-- local sliderName = getEntityName(entityId)
local settingKey = getEntityName(entityId)
local minX = -0.32
local maxX = 0.32
local hitHalfWidth = 0.05
local hitHalfHeight = 0.05

local dragging = false
local knobY = nil

-- Place functions to update volume here
local volumeFunctions = {
    master_handle = function(v)
        -- !TODO: These are example functions; replace them with your binded volume update functions
        -- globalBGMSetVolume(0, v)
    end,

    bgm_handle = function(v)
        -- globalBGMSetVolume(1, v)
    end,

    sfx_handle = function(v)
        -- globalBGMSetVolume(2, v)
    end
}

_G.SliderUI = _G.SliderUI or {}

-- Prevents the slider from going out of bounds
local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

-- Converts mouse position to UI Position
local function mouseToUI(mx, my)
    local fbW, fbH = getFrameBufferSize()

    if not fbW or not fbH or fbW == 0 or fbH == 0 then
        return 0.0, 0.0
    end

    local uiX = (mx / fbW) * 2.0 - 1.0
    local uiY = 1.0 - (my / fbH) * 2.0
    return uiX, uiY
end

-- Checks if mouse x,y (mx, my) is over selected x,y
local function isMouseOver(mx, my, x, y)
    return mx >= (x - hitHalfWidth) and mx <= (x + hitHalfWidth)
       and my >= (y - hitHalfHeight) and my <= (y + hitHalfHeight)
end

-- Checks distance between current position and selected value
local function positionToValue(x)
    if maxX == minX then
        return 0.0
    end
    return clamp((x - minX) / (maxX - minX), 0.0, 1.0)
end

registerUpdate(function(dt)
    local x, y = get2DPosition(entityId)

    if not knobY then
        knobY = y
    end

    local mx, my = getMousePos()
    local uiMouseX, uiMouseY = mouseToUI(mx, my)

    -- log("[UISlider dbg] mouse=", tostring(mx), tostring(my),
    --     " uiMouse=", tostring(uiMouseX), tostring(uiMouseY),
    --     " knob=", tostring(x), tostring(y))

    -- When slider is dragged
    if not dragging and isMouseDown(0) and isMouseOver(uiMouseX, uiMouseY, x, y) then
        dragging = true
        -- log("[UISlider] drag start")

        -- log("[UISlider] drag start, entityId=", tostring(entityId), " name=", tostring(sliderName))
    end

    -- When slider is released
    if wasMouseReleased(0) then
        dragging = false
        -- log("[UISlider] drag end")
    end

    -- Update slider position
    if dragging and isMouseDown(0) then
        local newX = clamp(uiMouseX, minX, maxX)
        set2DPosition(entityId, newX, knobY)

        local value = positionToValue(newX)
        _G.SliderUI[settingKey] = value

        log(string.format("[UISlider] %s = %.2f", settingKey, value))

        -- Calls the volume function according to the slider
        local fn = volumeFunctions[settingKey]
        if fn then
            fn(value)
        end

        -- !TODO: Code below not used unless things go wrong. Remove this code otherwise.
        -- Update volume here
        -- if sliderName == "master_handle" then
        --     -- Set master volume to value
        -- elseif sliderName == "bgm_handle" then
        --     -- Set bgm volume to value
        -- elseif sliderName == "sfx_handle" then
        --     -- Set sfx volume to value
        -- end

    end
end)