-- UIDetection.lua
-- Controls the detection bar, red overlay, and audio.

local ui = {}

-- config
local DURATION = 1.5    -- seconds to be “seen” before losing a life
local HIDE_X   = -2000  -- off-screen position to hide UI
local HIDE_Y   = -2000

local barBG       = findEntity("UI_DetectBar_BG")
local barFillL    = findEntity("UI_DetectBar_Fill_L")
local barFillR    = findEntity("UI_DetectBar_Fill_R")
local overlay     = findEntity("UI_DetectOverlay")

local cached = false
local bgX, bgY = 0, 0
local fillLX, fillLY = 0, 0
local fillRX, fillRY = 0, 0
local ovX, ovY = 0, 0
local FILL_HALF_WIDTH = 0.055

local preloadStage = 0
local PRELOAD_X = -1000
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

-- SFX file paths (no entity required)
local SFX_ALERT_HIT  = "game/audio/sfx/enemy/Enemy_Alert_Hit_v1.wav"
local SFX_ALERT_LOOP = "game/audio/sfx/enemy/Enemy_Alert_Loop_v1.wav"
local alertLoopChannel = -1

-- SFX volumes
local VOL_ALERT_HIT  = 0.8
local VOL_ALERT_LOOP = 0.7

-- internal state
ui.active         = false
ui.timer          = 0.0
ui.duration       = DURATION
ui.sourceEnemy    = nil
ui.isDetected     = false
ui.autoConfirmHit = true   -- NEW: true = old light-enemy behavior

-- SFX cooldown
local ALERT_SFX_COOLDOWN_TIME = 1.0
local alertSfxCooldown = 0.0

local DANGER_HAPTIC_INTERVAL = 0.65
local DANGER_HAPTIC_DURATION_MS = 40
local DANGER_HAPTIC_AMPLITUDE = 220
local dangerHapticTimer = 0.0

-- helpers -------------------------------------------------------

local function hideUI()
    cachePositions()
    if barBG then    set2DPosition(barBG, HIDE_X, HIDE_Y) end
    if barFillL then set2DPosition(barFillL, HIDE_X, HIDE_Y) end
    if barFillR then set2DPosition(barFillR, HIDE_X, HIDE_Y) end
    if overlay then  set2DPosition(overlay, HIDE_X, HIDE_Y) end
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
    if alertLoopChannel >= 0 then
        audioStopChannel(alertLoopChannel)
        if _G.SFXChannels then _G.SFXChannels[alertLoopChannel] = nil end
        alertLoopChannel = -1
    end

    -- Combat BGM transition disabled (game design decision)
    -- if _G.GlobalAudio and _G.GlobalAudio.setCombat then
    --     _G.GlobalAudio.setCombat(false)
    -- end
end

local function triggerDangerHapticPulse()
    if hapticsDangerPulse then
        hapticsDangerPulse(DANGER_HAPTIC_DURATION_MS, DANGER_HAPTIC_AMPLITUDE)
    end
end

-- public API called from enemy scripts --------------------------

-- Stops the detection audio without cancelling the overlay.
-- Call this after ui.begin() when the caller manages its own audio.
function ui.stopAudio()
    stopAudio()
end

-- autoConfirmHit:
--   true/nil  -> old behavior (fill bar, confirm hit after duration)
--   false     -> feedback only (overlay/audio, no auto damage)
function ui.begin(enemyEntity, autoConfirmHit)
    if autoConfirmHit == nil then
        autoConfirmHit = true
    end

    if ui.active then
        ui.sourceEnemy = enemyEntity
        ui.isDetected  = true
        -- autoConfirmHit=true (fill bar) always takes priority over false (feedback-only).
        -- This prevents the ground-chase enemy from suppressing the bar when a
        -- charge-bar enemy (light) is also detecting the player simultaneously.
        if autoConfirmHit then
            if not ui.autoConfirmHit then
                -- Upgrading from feedback-only to charge-bar: restore the bar
                -- background that the ground-chase enemy may have moved off-screen.
                showUI()
            end
            ui.autoConfirmHit = true
        end
        return
    end

    ui.active         = true
    ui.isDetected     = true
    ui.timer          = 0.0
    ui.sourceEnemy    = enemyEntity
    ui.autoConfirmHit = autoConfirmHit

    dangerHapticTimer = 0.0

    showUI()
    triggerDangerHapticPulse()

    if alertSfxCooldown <= 0 then
        audioPlaySFX(SFX_ALERT_HIT, VOL_ALERT_HIT)
        alertLoopChannel = audioPlaySFX(SFX_ALERT_LOOP, VOL_ALERT_LOOP, true)
        alertSfxCooldown = ALERT_SFX_COOLDOWN_TIME

        if alertLoopChannel >= 0 then
            _G.SFXChannels = _G.SFXChannels or {}
            _G.SFXChannels[alertLoopChannel] = true
        end
    end

    -- Combat BGM transition disabled (game design decision)
    -- if _G.GlobalAudio and _G.GlobalAudio.setCombat then
    --     _G.GlobalAudio.setCombat(true)
    -- end
end

function ui.cancel(enemyEntity)
    if enemyEntity and ui.sourceEnemy and enemyEntity ~= ui.sourceEnemy then
        return
    end

    if not ui.active then return end
    ui.isDetected  = false
    ui.sourceEnemy = nil
    dangerHapticTimer = 0.0
end

function ui.confirmHit()
    if not ui.active then return end
    ui.active     = false
    ui.isDetected = false
    ui.timer      = 0.0
    dangerHapticTimer = 0.0

    hideUI()
    stopAudio()

    if _G.PlayerState and _G.PlayerState.onCaught and _G.PlayerState.player then
        _G.PlayerState.onCaught(_G.PlayerState.player)
    end
end

-- per-frame update ----------------------------------------------

local function update(dt)
    if alertSfxCooldown > 0 then
        alertSfxCooldown = alertSfxCooldown - dt
    end

    if preloadStage < 5 then
        preloadStage = preloadStage + 1

        if preloadStage == 1 then
            if sfxAlertOnce then
                audioSetVolumeDb(sfxAlertOnce, -80.0)
                audioPlay(sfxAlertOnce)
            end
            if sfxAlertLoop then
                audioSetVolumeDb(sfxAlertLoop, -80.0)
                audioPlay(sfxAlertLoop)
            end

        elseif preloadStage == 2 then
            if sfxAlertOnce then audioStop(sfxAlertOnce) end
            if sfxAlertLoop then audioStop(sfxAlertLoop) end

        elseif preloadStage == 3 then
            cachePositions()
            if barBG then set2DPosition(barBG, PRELOAD_X, PRELOAD_Y) end
            if barFillL then set2DPosition(barFillL, PRELOAD_X, PRELOAD_Y) end
            if barFillR then set2DPosition(barFillR, PRELOAD_X, PRELOAD_Y) end
            if overlay then set2DPosition(overlay, PRELOAD_X, PRELOAD_Y) end

        elseif preloadStage == 4 then
            -- let textures render

        elseif preloadStage == 5 then
            hideUI()
            if barFillL then setScale(barFillL, 0.0, 1.0, 1.0) end
            if barFillR then setScale(barFillR, 0.0, 1.0, 1.0) end
        end

        return
    end

    if not ui.active then
        dangerHapticTimer = 0.0
        return
    end

    if ui.isDetected then
        dangerHapticTimer = dangerHapticTimer - dt
        if dangerHapticTimer <= 0.0 then
            triggerDangerHapticPulse()
            dangerHapticTimer = DANGER_HAPTIC_INTERVAL
        end
    else
        dangerHapticTimer = 0.0
    end

    -- auto-confirm mode: old light enemy behavior
    if ui.autoConfirmHit then
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
    else
        -- feedback-only mode: no auto hit, no bar charge logic
        if not ui.isDetected then
            ui.active = false
            ui.timer  = 0.0
            hideUI()
            stopAudio()
            return
        end

        ui.timer = 0.0
    end

    local t = ui.timer / ui.duration
    local clamped = math.max(0.0, math.min(0.88, t))

    if ui.autoConfirmHit then
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
    else
        -- feedback-only mode: keep bar empty
        if barFillL then
            local sx, sy = getUITextureScale(barFillL)
            setUITextureScale(barFillL, 0.0, sy)
        end

        if barFillR then
            local sx, sy = getUITextureScale(barFillR)
            setUITextureScale(barFillR, 0.0, sy)
        end
    end

    if alertLoopChannel >= 0 and ui.sourceEnemy then
        local ex, ey, ez = getPosition(ui.sourceEnemy)
        if audioSetChannelPosition then
            audioSetChannelPosition(alertLoopChannel, ex, ey, ez)
        end
    end
end

registerUpdate(update)

DetectionUI = ui
if _G_root then
    _G_root.DetectionUI = ui
end
