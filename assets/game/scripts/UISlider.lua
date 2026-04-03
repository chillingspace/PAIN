-- UISlider.lua
-- Attach to slider handle entity

-- local sliderName = getEntityName(entityId)
local settingKey = getEntityName(entityId)
local minX = -0.25
local maxX = 0.5
local hitHalfWidth = 0.05
local hitHalfHeight = 0.05

local dragging = false
local knobY = nil
local initialized = false

-- ==================== Volume helpers ====================

-- Convert linear 0.0-1.0 to decibels (0.0 -> silence, 1.0 -> 0 dB)
local function linearToDb(v)
    if v <= 0.001 then return -80.0 end
    return 20.0 * math.log(v) / math.log(10)
end

-- Convert a 0.0-1.0 value to slider X position
local function valueToPosition(v)
    return minX + v * (maxX - minX)
end

-- Test SFX played when the SFX volume slider is released
local SFX_TEST_FILE = "game/audio/sfx/Gear Pick Up.wav"

-- Settings key mapping (slider entity name -> settings file key)
local SETTINGS_KEYS = {
    master_handle     = "vol_master",
    bgm_handle        = "vol_bgm",
    sfx_handle        = "vol_sfx",
    brightness_handle = "gfx_brightness",
    gamma_handle      = "gfx_gamma",
}

-- Default slider values (0.0-1.0 range, maps to actual value via handler)
local DEFAULT_VALUES = {
    master_handle     = 1.0,
    bgm_handle        = 1.0,
    sfx_handle        = 1.0,
    brightness_handle = 0.333,  -- maps to exposure 1.0 (range 0.5-2.0)
    gamma_handle      = 0.467,  -- maps to gamma 2.2 (range 1.5-3.0)
}

-- Global volume state (persists across scenes via _G)
_G.VolumeSettings = _G.VolumeSettings or {
    master = 1.0,
    bgm    = 1.0,
    sfx    = 1.0,
}

-- ==================== Slider -> volume mapping ====================

-- Place functions to update volume here
local volumeFunctions = {
    master_handle = function(v)
        _G.VolumeSettings.master = v
        if audioSetGroupVolumeDb then
            audioSetGroupVolumeDb("master", linearToDb(v))
        end
    end,

    bgm_handle = function(v)
        _G.VolumeSettings.bgm = v
        if audioSetGroupVolumeDb then
            audioSetGroupVolumeDb("music", linearToDb(v))
        end
    end,

    sfx_handle = function(v)
        _G.VolumeSettings.sfx = v
        if audioSetGroupVolumeDb then
            audioSetGroupVolumeDb("sfx", linearToDb(v))
        end
    end,

    brightness_handle = function(v)
        -- Map 0.0-1.0 slider to 0.5-2.0 exposure
        local exposure = 0.5 + v * 1.5
        if setBrightness then
            setBrightness(exposure)
            local readback = getBrightness and getBrightness() or -999
            log(string.format("[BRIGHTNESS DEBUG] set exposure=%.3f, readback=%.3f", exposure, readback))
        else
            log("[BRIGHTNESS DEBUG] setBrightness function NOT FOUND!")
        end
    end,

    gamma_handle = function(v)
        -- Map 0.0-1.0 slider to 1.5-3.0 gamma
        local gamma = 1.5 + v * 1.5
        if setGamma then
            setGamma(gamma)
            local readback = getGamma and getGamma() or -999
            log(string.format("[GAMMA DEBUG] set gamma=%.3f, readback=%.3f", gamma, readback))
        else
            log("[GAMMA DEBUG] setGamma function NOT FOUND!")
        end
    end
}

_G.SliderUI = _G.SliderUI or {}

-- ==================== Utility ====================

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

    -- ==================== INIT: Restore slider from saved settings ====================
    if not initialized then
        initialized = true
        local fileKey = SETTINGS_KEYS[settingKey]
        if fileKey and settingsLoad then
            local saved = settingsLoad(fileKey, "")
            if saved ~= "" then
                local val = tonumber(saved)
                if val then
                    val = clamp(val, 0.0, 1.0)
                    -- Move the slider knob to the saved position
                    local newX = valueToPosition(val)
                    set2DPosition(entityId, newX, knobY)
                    -- Apply the volume immediately
                    _G.SliderUI[settingKey] = val
                    local fn = volumeFunctions[settingKey]
                    if fn then fn(val) end
                    log(string.format("[UISlider] Restored %s = %.2f from settings", settingKey, val))
                end
            end
        end
    end

    local mx, my = getMousePos()
    local uiMouseX, uiMouseY = mouseToUI(mx, my)

    -- When slider is dragged
    if not dragging and isMouseDown(0) and isMouseOver(uiMouseX, uiMouseY, x, y) then
        dragging = true
    end

    -- When slider is released
    if dragging and wasMouseReleased(0) then
        dragging = false

        -- Save the current value to disk
        local finalValue = _G.SliderUI[settingKey]
        local fileKey = SETTINGS_KEYS[settingKey]
        if finalValue and fileKey and settingsSave then
            settingsSave(fileKey, string.format("%.4f", finalValue))
            log(string.format("[UISlider] Saved %s = %.4f to settings", fileKey, finalValue))
        end

        -- Play test SFX when the SFX volume slider is released
        if settingKey == "sfx_handle" and audioPlaySFX then
            audioPlaySFX(SFX_TEST_FILE, 0.0)
        end
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
    end
end)