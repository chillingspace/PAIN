-- UIDetection.lua
-- Controls the detection bar, red overlay, and audio.

local ui = {}

-- config
local DURATION = 1.5    -- seconds to be “seen” before losing a life
local HIDE_X   = -2000  -- off-screen position to hide UI
local HIDE_Y   = -2000

local barBG       = findEntity("UI_DetectBar_BG")
local barFillL   = findEntity("UI_DetectBar_Fill_L")
local barFillR   = findEntity("UI_DetectBar_Fill_R")
local overlay     = findEntity("UI_DetectOverlay")

local cached = false
local bgX, bgY = 0, 0
local fillLX, fillLY = 0, 0
local fillRX, fillRY = 0, 0
local ovX, ovY = 0, 0
local FILL_HALF_WIDTH = 0.055 -- was 0.05

local preloadStage = 0
local PRELOAD_X = -1000  -- different from HIDE_X to force a position change
local PRELOAD_Y = -1000

local INNER_OFFSET = 0.006

local function cachePositions()
    if cached then return end
    if barBG then bgX, bgY = get2DPosition(barBG) end
    if barFillL then fillLX, fillLY = get2DPosition(barFillL) end
    if barFillR then fillRX, fillRY = get2DPosition(barFillR) end
    if overlay then ovX, ovY = get2DPosition(overlay) end
    cached = true
end

-- audio entities (SFX only - combat BGM now handled by GlobalAudio)
local sfxAlertOnce   = findEntity("Enemy_Alert_Hit_v1")
local sfxAlertLoop   = findEntity("Enemy_Alert_Loop_v1")
-- bgmCombatLayer is now controlled via GlobalAudio.setCombat()

-- internal state
ui.active      = false
ui.timer       = 0.0
ui.duration    = DURATION
ui.sourceEnemy = nil
ui.isDetected  = false

-- SFX volumes
local ALERT_ONCE_DB = -8.0   
local ALERT_LOOP_DB = -12.0

-- helpers -------------------------------------------------------

local function hideUI()
    cachePositions()
    if barBG then    set2DPosition(barBG,   HIDE_X, HIDE_Y)  end
    if barFillL then set2DPosition(barFillL, HIDE_X, HIDE_Y) end
    if barFillR then set2DPosition(barFillR, HIDE_X, HIDE_Y) end
    if overlay then  set2DPosition(overlay, HIDE_X, HIDE_Y)  end
end

local function showUI()
    cachePositions()
    if barBG then set2DPosition(barBG, bgX, bgY) end

    if barFillL then
        set2DPosition(barFillL, fillLX, fillLY)
        setScale(barFillL, 0.0, 1.0, 1.0)
    end

    if barFillR then
        set2DPosition(barFillR, fillRX, fillRY)
        setScale(barFillR, 0.0, 1.0, 1.0)
    end

    if overlay then set2DPosition(overlay, ovX, ovY) end
end

local function stopAudio()
    if sfxAlertLoop   then audioStop(sfxAlertLoop)   end
    -- Combat BGM now controlled via GlobalAudio.setCombat(false)
    if _G.GlobalAudio and _G.GlobalAudio.setCombat then
        _G.GlobalAudio.setCombat(false)
    end
end

-- public API called from enemy scripts --------------------------

function ui.begin(enemyEntity)
    if ui.active then
        ui.sourceEnemy = enemyEntity
        return
    end

    ui.active      = true
    ui.isDetected  = true
    ui.timer       = 0.0
    ui.sourceEnemy = enemyEntity

    showUI()

    -- enemy alert SFX once then loop
    if sfxAlertOnce then 
        audioSetVolumeDb(sfxAlertOnce, ALERT_ONCE_DB)
        audioPlay(sfxAlertOnce) 
    end

    if sfxAlertLoop then
        audioSetLooping(sfxAlertLoop, true)
        audioSetVolumeDb(sfxAlertLoop, ALERT_LOOP_DB)
        audioPlay(sfxAlertLoop)
    end

    -- Start combat BGM layer via GlobalAudio
    if _G.GlobalAudio and _G.GlobalAudio.setCombat then
        _G.GlobalAudio.setCombat(true)
    end
end

function ui.cancel(enemyEntity)
    -- only cancel if the same enemy that started it
    if enemyEntity and ui.sourceEnemy and enemyEntity ~= ui.sourceEnemy then
        return
    end

    if not ui.active then return end
    ui.isDetected  = false
    ui.sourceEnemy = nil
end

-- When the bar fully fills
function ui.confirmHit()
    if not ui.active then return end
    ui.active     = false
    ui.isDetected = false
    ui.timer      = 0.0

    hideUI()
    stopAudio()

    -- tell global player state script that player got caught
    if _G.PlayerState and _G.PlayerState.onCaught and _G.PlayerState.player then
        _G.PlayerState.onCaught(_G.PlayerState.player)
    end
end

-- per-frame update ----------------------------------------------

local function update(dt)
    if preloadStage < 5 then
        preloadStage = preloadStage + 1
        
        if preloadStage == 1 then
            -- Frame 1: preload audio
            if sfxAlertOnce then
                audioSetVolumeDb(sfxAlertOnce, -80.0)
                audioPlay(sfxAlertOnce)
            end
            if sfxAlertLoop then
                audioSetVolumeDb(sfxAlertLoop, -80.0)
                audioPlay(sfxAlertLoop)
            end
        elseif preloadStage == 2 then
            -- Frame 2: stop audio
            if sfxAlertOnce then audioStop(sfxAlertOnce) end
            if sfxAlertLoop then audioStop(sfxAlertLoop) end
        
        elseif preloadStage == 3 then
            -- Frame 3: move UI to a preload position (still off-screen)
            cachePositions()
            if barBG then set2DPosition(barBG, PRELOAD_X, PRELOAD_Y) end
            if barFillL then 
                set2DPosition(barFillL, PRELOAD_X, PRELOAD_Y)
            end
            if barFillR then 
                set2DPosition(barFillR, PRELOAD_X, PRELOAD_Y) 
            end
            if overlay then set2DPosition(overlay, PRELOAD_X, PRELOAD_Y) end

        elseif preloadStage == 4 then
            -- Frame 4: let textures render

        elseif preloadStage == 5 then
            -- Frame 5: hide properly and reset scales
            hideUI()
            -- Reset texture scales to 0 so they're ready for first detection
            if barFillL then setScale(barFillL, 0.0, 1.0, 1.0) end
            if barFillR then setScale(barFillR, 0.0, 1.0, 1.0) end
        end
        
        return
    end


    if not ui.active then
        return
    end

    -- Charge up while detected, drain down when not
    if ui.isDetected then
        ui.timer = ui.timer + dt
        if ui.timer >= ui.duration then
            ui.timer = ui.duration
            ui.confirmHit()
            return
        end
    else
        ui.timer = ui.timer - dt
        if ui.timer <= 0.0 then
            ui.timer  = 0.0
            ui.active = false
            hideUI()
            stopAudio()
            return
        end
    end

    local t = ui.timer / ui.duration

    local clamped = math.max(0.0, math.min(0.88, t)) 
    -- if barFillL then setScale(barFillL, clamped, 1.0, 1.0) end
    -- if barFillR then setScale(barFillR, clamped, 1.0, 1.0) end

    if barFillL then
        local sx, sy = getUITextureScale(barFillL)
        setUITextureScale(barFillL, clamped, sy)

        local shift = (1.0 - clamped) * FILL_HALF_WIDTH
        set2DPosition(barFillL, fillLX - shift + INNER_OFFSET, fillLY)
    end

    if barFillR then
        local sx, sy = getUITextureScale(barFillR)
        setUITextureScale(barFillR, clamped, sy)

        local shift = (1.0 - clamped) * FILL_HALF_WIDTH
        set2DPosition(barFillR, fillRX + shift - INNER_OFFSET, fillRY)
    end


    -- Combat BGM is now controlled via GlobalAudio.setCombat, no per-frame fade needed here
end

registerUpdate(update)

-- -- start hidden on scene load
-- hideUI()

-- make this table globally accessible to other scripts
DetectionUI = ui -- normal global
if _G_root then
    _G_root.DetectionUI = ui
end