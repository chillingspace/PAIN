--[[
    GlobalAudioController.lua
    
    Controls persistent global audio that survives scene changes.
    Attach this script to a Global_BGM entity in each scene.
    
    Lua API available:
    - globalBGMSetVolume(trackIndex, volumeDb)
    - globalBGMFade(trackIndex, targetDb, durationSeconds)
    - globalBGMGetTrackCount()
    - globalBGMGetVolume(trackIndex)
    - globalBGMStopAll()
    - globalBGMClear()
    - globalBGMIsInitialized()
    - globalBGMFadeAllAndQuit(durationSeconds)
    
    Track Layout:
    - mainmenu.scn: Track 0 = Main BGM, Track 1 = Ambient
    - Level1.scn: Track 0 = Level BGM, Track 1 = Ambient, Track 2 = Combat BGM
]]

-- ==================== USER SETTINGS ====================
-- Control the volume of the Main Menu BGM here (0.0 to 1.0)
local MAINMENU_BGM_VOLUME_SCALE = 0.5
local TUTORIAL_BGM_VOLUME_SCALE = 0.25
local LEVEL1_BGM_VOLUME_SCALE   = 0.25
local LEVEL2_BGM_VOLUME_SCALE   = 0.25

-- ==================== HELPERS ====================
local function linearToDb(linear)
    if linear <= 0.0001 then return -80.0 end
    -- Polyfill for log10
    return 20.0 * (math.log(linear) / math.log(10))
end

-- ==================== CONFIGURATION ====================
local CONFIG = {
    -- Fade durations (seconds)
    fadeDuration = 1.0,           -- Fade-in after scene loads (was 2.0)
    crossfadeDuration = 1.5,      -- Combat crossfade duration
    quitFadeDuration = 2.0,       -- Fade-out when quitting game
    sceneTransitionDelay = 1.5,   -- Delay before scene actually changes (fade-out time)
    
    -- Default volumes (dB)
    defaultVolume = 0.0,    -- Full volume
    ambientVolume = -6.0,   -- Ambient slightly quieter
    mutedVolume = -80.0     -- Muted
}

-- ==================== SCENE DETECTION ====================
local function detectCurrentScene()
    local sceneName = "unknown"
    if getCurrentSceneName then
        sceneName = getCurrentSceneName()
        if sceneName then
            log("[GlobalAudioDebug] Raw Scene Name: " .. tostring(sceneName))
            local lowerName = string.lower(sceneName)
            if string.find(lowerName, "mainmenu") then return "mainmenu" end
            if string.find(lowerName, "cutscene") then return "cutscene" end
            if string.find(lowerName, "tutorial") then return "tutorial" end
            if string.find(lowerName, "howtoplay2") then return "howtoplay2" end
            if string.find(lowerName, "howtoplay") then return "howtoplay" end
            if string.find(lowerName, "level1") then return "level1" end
            if string.find(lowerName, "level 2") or string.find(lowerName, "level2") then return "level2" end
            if string.find(lowerName, "level") then return "level" end
        else
            log("[GlobalAudioDebug] getCurrentSceneName returned nil")
        end
    else
        log("[GlobalAudioDebug] getCurrentSceneName function NOT found")
    end
    return "unknown"
end

-- ==================== STATE ====================
local currentScene = nil
local initialized = false
local inCombat = false

-- ==================== SCENE-SPECIFIC VOLUME CONFIG ====================
local function applySceneVolumes()
    local trackCount = globalBGMGetTrackCount()
    
    if currentScene == "mainmenu" then
        -- Track 0: Main BGM, Track 1: Ambient
        local mainBGMVolume = linearToDb(MAINMENU_BGM_VOLUME_SCALE)
        if trackCount >= 1 then globalBGMFade(0, mainBGMVolume, CONFIG.fadeDuration) end
        
    elseif currentScene == "howtoplay" or currentScene == "howtoplay2" then
        -- howtoplay has only MainBGM (1 track) - Ambient was stopped by C++
        local mainBGMVolume = linearToDb(MAINMENU_BGM_VOLUME_SCALE)
        if trackCount >= 1 then globalBGMFade(0, mainBGMVolume, CONFIG.fadeDuration) end
        
    elseif currentScene == "cutscene" then
        -- Cutscene uses same main BGM as mainmenu/tutorial
        local mainBGMVolume = linearToDb(MAINMENU_BGM_VOLUME_SCALE)
        if trackCount >= 1 then globalBGMFade(0, mainBGMVolume, CONFIG.fadeDuration) end
        for i = 1, trackCount - 1 do
            globalBGMFade(i, CONFIG.mutedVolume, 0.1)
        end
        
    elseif currentScene == "tutorial" or currentScene == "level1" or currentScene == "level2" or currentScene == "level" then
        -- Determine target volume based on scene
        local targetVol = CONFIG.defaultVolume
        if currentScene == "tutorial" then targetVol = linearToDb(TUTORIAL_BGM_VOLUME_SCALE) end
        if currentScene == "level1"   then targetVol = linearToDb(LEVEL1_BGM_VOLUME_SCALE) end
        if currentScene == "level2"   then targetVol = linearToDb(LEVEL2_BGM_VOLUME_SCALE) end

        -- Track 0: Level BGM (main), Track 1: Ambient, Track 2: Combat (muted initially)
        if trackCount >= 1 then globalBGMFade(0, targetVol, CONFIG.fadeDuration) end
        if trackCount >= 2 then globalBGMFade(1, CONFIG.ambientVolume, CONFIG.fadeDuration) end
        -- Combat BGM starts muted; enabled via GlobalAudio_SetCombat when enemies chase
        if trackCount >= 3 then globalBGMFade(2, CONFIG.mutedVolume, 0.1) end
        inCombat = false
        
    else
        -- Unknown scene: fade in all tracks to default
        for i = 0, trackCount - 1 do
            globalBGMFade(i, CONFIG.defaultVolume, CONFIG.fadeDuration)
        end
    end
end

-- ==================== GLOBAL SFX CLEANUP ====================
-- Stops all tracked SFX channels (looping sounds registered by scripts)
local function stopAllSFXChannels()
    if _G.SFXChannels and audioStopChannel then
        for channelId, _ in pairs(_G.SFXChannels) do
            audioStopChannel(channelId)
        end
        _G.SFXChannels = {}
    end
    log("[GlobalAudio] Stopped all tracked SFX channels")
end

-- ==================== FADE OUT ALL TRACKS ====================
local function fadeOutAllTracks()
    local trackCount = globalBGMGetTrackCount()
    for i = 0, trackCount - 1 do
        globalBGMFade(i, CONFIG.mutedVolume, CONFIG.sceneTransitionDelay - 0.2)
    end
    -- Stop all SFX on scene transition
    stopAllSFXChannels()
end

-- ==================== DELAYED SCENE CHANGE ====================
-- Call this instead of changeScene() to get a fade-out first
local function changeSceneWithFade(scenePath)
    if not scenePath then
        return
    end
    
    -- Fade out all global audio tracks
    fadeOutAllTracks()
    
    -- Delay the actual scene change
    setTimeout(function()
        if changeScene then
            changeScene(scenePath)
        end
    end, CONFIG.sceneTransitionDelay)
end

-- ==================== COMBAT LAYER CONTROL ====================
-- Combat BGM transition disabled (game design decision)
-- Call this from UIDetection.lua or enemy scripts
function GlobalAudio_SetCombat(combatActive)
    -- Combat BGM crossfade only active in level2
    if currentScene ~= "level2" then return end
    if combatActive == inCombat then return end

    inCombat = combatActive
    local trackCount = globalBGMGetTrackCount()

    if combatActive then
        -- Fade out main BGM, fade in combat layer (Track 2)
        if trackCount >= 1 then globalBGMFade(0, CONFIG.mutedVolume, CONFIG.crossfadeDuration) end
        if trackCount >= 3 then globalBGMFade(2, CONFIG.defaultVolume, CONFIG.crossfadeDuration) end
    else
        -- Fade in main BGM, fade out combat layer
        local targetVol = linearToDb(LEVEL2_BGM_VOLUME_SCALE)
        if trackCount >= 1 then globalBGMFade(0, targetVol, CONFIG.crossfadeDuration) end
        if trackCount >= 3 then globalBGMFade(2, CONFIG.mutedVolume, CONFIG.crossfadeDuration) end
    end
end

-- ==================== GAME OVER DUCKING ====================
function GlobalAudio_SetGameOver(active)
    -- Only relevant for Tutorial and Level1 where we have BGM + Game Over Loop
    if currentScene ~= "tutorial" and currentScene ~= "level1" and currentScene ~= "level2" and currentScene ~= "level" then return end
    
    local trackCount = globalBGMGetTrackCount()
    if trackCount < 1 then return end

    if active then
        -- Duck Main BGM (Track 0) to 0.1 linear volume
        local duckVol = linearToDb(0.0)
        globalBGMFade(0, duckVol, 0.5)
        log("[GlobalAudio] Game Over: Ducking BGM to 0.1")
    else
        -- Restore original volume
        local targetVol = CONFIG.defaultVolume
        if currentScene == "tutorial" then targetVol = linearToDb(TUTORIAL_BGM_VOLUME_SCALE) end
        if currentScene == "level1"   then targetVol = linearToDb(LEVEL1_BGM_VOLUME_SCALE) end
        if currentScene == "level2"   then targetVol = linearToDb(LEVEL2_BGM_VOLUME_SCALE) end

        globalBGMFade(0, targetVol, 1.0)
        log("[GlobalAudio] Game Over Ended: Restoring BGM")
    end
end

-- ==================== PUBLIC API ====================
_G.GlobalAudio = _G.GlobalAudio or {}
_G.GlobalAudio.prepareQuit = GlobalAudio_PrepareQuit
_G.GlobalAudio.setCombat = GlobalAudio_SetCombat
_G.GlobalAudio.setGameOver = GlobalAudio_SetGameOver
_G.GlobalAudio.changeSceneWithFade = changeSceneWithFade
_G.GlobalAudio.fadeOutAllTracks = fadeOutAllTracks
_G.GlobalAudio.stopAllSFXChannels = stopAllSFXChannels
_G.GlobalAudio.getConfig = function() return CONFIG end
_G.GlobalAudio.getCurrentScene = function() return currentScene end
_G.GlobalAudio.isInCombat = function() return inCombat end

-- ==================== INITIALIZATION ====================
-- Run immediately when script loads
currentScene = detectCurrentScene()

-- Wait a few frames before applying volumes
local frameCount = 0
local FRAMES_TO_WAIT = 3

registerUpdate(function(dt)
    if not initialized then
        frameCount = frameCount + 1
        if frameCount >= FRAMES_TO_WAIT then
            initialized = true
            applySceneVolumes()

            -- Apply saved volume settings from disk (runs once on game startup)
            if settingsLoad and audioSetGroupVolumeDb then
                local function loadAndApply(fileKey, group, globalKey)
                    local saved = settingsLoad(fileKey, "")
                    if saved ~= "" then
                        local val = tonumber(saved)
                        if val then
                            if val <= 0.001 then val = 0.001 end  -- clamp minimum
                            local db = 20.0 * math.log(val) / math.log(10)
                            if val <= 0.001 then db = -80.0 end
                            audioSetGroupVolumeDb(group, db)
                            _G.VolumeSettings = _G.VolumeSettings or {}
                            _G.VolumeSettings[globalKey] = val
                            log("[GlobalAudio] Loaded " .. fileKey .. " = " .. tostring(val))
                        end
                    end
                end
                loadAndApply("vol_master", "master", "master")
                loadAndApply("vol_bgm",    "music",  "bgm")
                loadAndApply("vol_sfx",    "sfx",    "sfx")
            end

            -- Apply saved graphics settings from disk
            if settingsLoad then
                -- Brightness (tone mapping exposure)
                if setBrightness then
                    local saved = settingsLoad("gfx_brightness", "")
                    if saved ~= "" then
                        local val = tonumber(saved)
                        if val then
                            -- Convert slider 0-1 to exposure 0.5-2.0
                            local exposure = 0.5 + val * 1.5
                            setBrightness(exposure)
                            log("[GlobalAudio] Loaded brightness slider = " .. tostring(val) .. " -> exposure " .. tostring(exposure))
                        end
                    end
                end

                -- Gamma
                if setGamma then
                    local saved = settingsLoad("gfx_gamma", "")
                    if saved ~= "" then
                        local val = tonumber(saved)
                        if val then
                            -- Convert slider 0-1 to gamma 1.5-3.0
                            local gamma = 1.5 + val * 1.5
                            setGamma(gamma)
                            log("[GlobalAudio] Loaded gamma slider = " .. tostring(val) .. " -> gamma " .. tostring(gamma))
                        end
                    end
                end

                -- Fullscreen/Windowed (disabled: always start windowed, don't persist across launches)
                -- if setFullscreen then
                --     local saved = settingsLoad("gfx_displaymode", "")
                --     if saved ~= "" then
                --         _G.GraphicsSettings = _G.GraphicsSettings or {}
                --         _G.GraphicsSettings.displayMode = saved
                --         setFullscreen(saved == "fullscreen")
                --         log("[GlobalAudio] Loaded display mode = " .. saved)
                --     end
                -- end
            end
        end
    end
end)
