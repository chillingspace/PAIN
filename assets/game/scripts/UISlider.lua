-- UISlider.lua
-- Attach to slider handle entity

local settingKey = "musicVolume"
local minX = -0.32
local maxX = 0.32
local hitHalfWidth = 0.05
local hitHalfHeight = 0.05

local dragging = false
local knobY = nil

_G.SliderUI = _G.SliderUI or {}

-- Prevents the slider from going out of bounds
local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

-- Checks if mouse x,y is over selected x,y
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

    -- DEBUG
    local mx, my = getMousePos()
    log("[UISlider dbg] mouse=", tostring(mx), tostring(my), " knob=", tostring(x), tostring(y))

    if not knobY then
        knobY = y
    end

    local mx, my = getMousePos()

    -- if wasMousePressed(0) and isMouseOver(mx, my, x, y) then
    --     dragging = true
    --     log("[ui_slider] drag start", tostring(entityId))
    -- end

    if not dragging and isMouseDown(0) and isMouseOver(mx, my, x, y) then
        dragging = true
        log("[UISlider] drag start")
    end

    if wasMouseReleased(0) then
        dragging = false
        log("[UISlider] drag end")
    end

    if dragging and isMouseDown(0) then
        local newX = clamp(mx, minX, maxX)
        set2DPosition(entityId, newX, knobY)

        local value = positionToValue(newX)
        _G.SliderUI[settingKey] = value

        log(string.format("[ui_slider] %s = %.2f", settingKey, value))
    end
end)