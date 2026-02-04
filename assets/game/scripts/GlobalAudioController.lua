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

-- ==================== CONFIGURATION ====================
local CONFIG = {
    -- Fade durations (seconds)
    fadeDuration = 2.0,
    crossfadeDuration = 2.5,
    quitFadeDuration = 2.0,
    sceneTransitionDelay = 2.0,  -- Delay before scene actually changes
    
    -- Default volumes (dB)
    defaultVolume = 0.0,    -- Full volume
    ambientVolume = -6.0,   -- Ambient slightly quieter
    mutedVolume = -80.0     -- Muted
}

-- ==================== SCENE DETECTION ====================
local function detectCurrentScene()
    if getCurrentSceneName then
        local sceneName = getCurrentSceneName()
        if sceneName then
            local lowerName = string.lower(sceneName)
            if string.find(lowerName, "mainmenu") then return "mainmenu" end
            if string.find(lowerName, "howtoplay2") then return "howtoplay2" end
            if string.find(lowerName, "howtoplay") then return "howtoplay" end
            if string.find(lowerName, "level1") then return "level1" end
            if string.find(lowerName, "level") then return "level" end
        end
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
    log("[GlobalAudio] Scene:" .. currentScene .. " Tracks:" .. tostring(trackCount) .. " Init:" .. tostring(globalBGMIsInitialized()))
    
    if currentScene == "mainmenu" then
        -- Track 0: Main BGM, Track 1: Ambient
        if trackCount >= 1 then globalBGMFade(0, CONFIG.defaultVolume, CONFIG.fadeDuration) end
        if trackCount >= 2 then globalBGMFade(1, CONFIG.ambientVolume, CONFIG.fadeDuration) end
        
    elseif currentScene == "howtoplay" or currentScene == "howtoplay2" then
        -- howtoplay scenes don't have their own Global_BGM tracks, 
        -- they just keep playing whatever was already initialized
        -- Don't fade anything - let mainmenu tracks continue
        log("[GlobalAudio] howtoplay scene - keeping existing tracks")
        
    elseif currentScene == "level1" or currentScene == "level" then
        -- Track 0: Level BGM, Track 1: Ambient, Track 2: Combat (muted initially)
        if trackCount >= 1 then globalBGMFade(0, CONFIG.defaultVolume, CONFIG.fadeDuration) end
        if trackCount >= 2 then globalBGMFade(1, CONFIG.ambientVolume, CONFIG.fadeDuration) end
        if trackCount >= 3 then globalBGMFade(2, CONFIG.mutedVolume, CONFIG.fadeDuration) end
        inCombat = false
        
    else
        -- Unknown scene: fade in all tracks
        for i = 0, trackCount - 1 do
            globalBGMFade(i, CONFIG.defaultVolume, CONFIG.fadeDuration)
        end
    end
end

-- ==================== FADE OUT ALL TRACKS ====================
local function fadeOutAllTracks()
    local trackCount = globalBGMGetTrackCount()
    log("[GlobalAudio] Fading out " .. tostring(trackCount) .. " tracks for transition")
    for i = 0, trackCount - 1 do
        globalBGMFade(i, CONFIG.mutedVolume, CONFIG.sceneTransitionDelay - 0.2)
    end
end

-- ==================== DELAYED SCENE CHANGE ====================
-- Call this instead of changeScene() to get a fade-out first
local function changeSceneWithFade(scenePath)
    if not scenePath then
        log("[GlobalAudio] Error: scenePath is nil")
        return
    end
    
    log("[GlobalAudio] Preparing scene transition to: " .. scenePath)
    
    -- Fade out all global audio tracks
    fadeOutAllTracks()
    
    -- Delay the actual scene change
    setTimeout(function()
        log("[GlobalAudio] Executing scene change to: " .. scenePath)
        if changeScene then
            changeScene(scenePath)
        end
    end, CONFIG.sceneTransitionDelay)
end

-- ==================== COMBAT LAYER CONTROL ====================
-- Call this from UIDetection.lua or enemy scripts
function GlobalAudio_SetCombat(combatActive)
    if currentScene ~= "level1" and currentScene ~= "level" then return end
    if combatActive == inCombat then return end
    
    inCombat = combatActive
    local trackCount = globalBGMGetTrackCount()
    
    if combatActive then
        -- Fade out main BGM, fade in combat layer
        if trackCount >= 1 then globalBGMFade(0, CONFIG.mutedVolume, CONFIG.crossfadeDuration) end
        if trackCount >= 3 then globalBGMFade(2, CONFIG.defaultVolume, CONFIG.crossfadeDuration) end
        log("[GlobalAudio] Combat ON - switching to combat BGM")
    else
        -- Fade in main BGM, fade out combat layer
        if trackCount >= 1 then globalBGMFade(0, CONFIG.defaultVolume, CONFIG.crossfadeDuration) end
        if trackCount >= 3 then globalBGMFade(2, CONFIG.mutedVolume, CONFIG.crossfadeDuration) end
        log("[GlobalAudio] Combat OFF - switching to normal BGM")
    end
end

-- ==================== QUIT HANDLING ====================
function GlobalAudio_PrepareQuit()
    log("[GlobalAudio] Preparing quit with fade")
    local trackCount = globalBGMGetTrackCount()
    for i = 0, trackCount - 1 do
        globalBGMFade(i, CONFIG.mutedVolume, CONFIG.quitFadeDuration)
    end
    setTimeout(function()
        quitApplication()
    end, CONFIG.quitFadeDuration + 0.3)
end

-- ==================== PUBLIC API ====================
_G.GlobalAudio = _G.GlobalAudio or {}
_G.GlobalAudio.prepareQuit = GlobalAudio_PrepareQuit
_G.GlobalAudio.setCombat = GlobalAudio_SetCombat
_G.GlobalAudio.changeSceneWithFade = changeSceneWithFade
_G.GlobalAudio.fadeOutAllTracks = fadeOutAllTracks
_G.GlobalAudio.getConfig = function() return CONFIG end
_G.GlobalAudio.getCurrentScene = function() return currentScene end
_G.GlobalAudio.isInCombat = function() return inCombat end

-- ==================== INITIALIZATION ====================
-- Run immediately when script loads
currentScene = detectCurrentScene()

-- Use registerUpdate to check once on first frame
registerUpdate(function(dt)
    if not initialized then
        initialized = true
        applySceneVolumes()
    end
end)
