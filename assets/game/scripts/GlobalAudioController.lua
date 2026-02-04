--[[
    GlobalAudioController.lua
    
    Controls persistent global audio that survives scene changes.
    Attach this script to a Global_BGM entity in each scene.
    
    Scene configurations:
    - mainmenu.scn: 2 tracks (BGM + layer), both unmuted
    - howtoplay.scn / howtoplay2.scn: No AudioSource, just script (keeps audio from previous scene)
    - Level1.scn: 3 tracks (BGM + ambience + layer), BGM and ambience unmuted
    
    Lua API available:
    - globalBGMSetVolume(trackIndex, volumeDb)
    - globalBGMFade(trackIndex, targetDb, durationSeconds)
    - globalBGMGetTrackCount()
    - globalBGMGetVolume(trackIndex)
    - globalBGMStopAll()
    - globalBGMClear()
    - globalBGMIsInitialized()
    - globalBGMFadeAllAndQuit(durationSeconds)
]]

-- ==================== CONFIGURATION ====================
-- Track indices for each scene type
-- Adjust these to match your actual audio track order in the editor

local CONFIG = {
    -- Main Menu has 2 tracks
    mainmenu = {
        trackCount = 2,
        BGM = 0,        -- Main background music
        LAYER = 1,      -- Additional layer (e.g., melody overlay)
        defaultVolumes = {
            [0] = 0,    -- BGM at 0dB (full volume)
            [1] = 0     -- Layer at 0dB
        }
    },
    
    -- Level1 has 3 tracks
    level1 = {
        trackCount = 3,
        BGM = 0,        -- Main gameplay BGM
        AMBIENCE = 1,   -- Ambient sounds
        LAYER = 2,      -- Dynamic layer
        defaultVolumes = {
            [0] = 0,    -- BGM at 0dB
            [1] = -6,   -- Ambience slightly quieter
            [2] = -80   -- Layer muted by default
        }
    },
    
    -- Fade durations (seconds)
    fadeDuration = 2.0,
    crossfadeDuration = 2.5,
    quitFadeDuration = 2.0
}

-- ==================== STATE ====================
local currentScene = nil
local previousScene = nil
local isTransitioning = false

-- ==================== SCENE DETECTION ====================
-- Detect which scene we're in based on available global functions or scene name
local function detectCurrentScene()
    -- Try to get scene name from engine
    local sceneName = getCurrentSceneName and getCurrentSceneName() or nil
    
    if sceneName then
        -- Normalize to lowercase for comparison
        sceneName = string.lower(sceneName)
        
        if string.find(sceneName, "mainmenu") then
            return "mainmenu"
        elseif string.find(sceneName, "howtoplay2") then
            return "howtoplay2"
        elseif string.find(sceneName, "howtoplay") then
            return "howtoplay"
        elseif string.find(sceneName, "level1") then
            return "level1"
        end
    end
    
    -- Fallback: check track count to determine scene type
    local trackCount = globalBGMGetTrackCount()
    if trackCount == 2 then
        return "mainmenu"  -- Or could be howtoplay, but those shouldn't have tracks
    elseif trackCount == 3 then
        return "level1"
    elseif trackCount == 0 then
        return "howtoplay"  -- Scenes without AudioSource on Global_BGM
    end
    
    return "unknown"
end

-- ==================== AUDIO CONTROL ====================

-- Fade in all tracks for a scene based on config
local function fadeInSceneTracks(sceneType)
    local config = CONFIG[sceneType]
    if not config then
        log("[GlobalAudio] Unknown scene type: " .. tostring(sceneType))
        return
    end
    
    local trackCount = globalBGMGetTrackCount()
    log("[GlobalAudio] Fading in " .. trackCount .. " tracks for scene: " .. sceneType)
    
    for i = 0, trackCount - 1 do
        local targetVol = config.defaultVolumes[i] or -80
        globalBGMFade(i, targetVol, CONFIG.fadeDuration)
        log("[GlobalAudio] Track " .. i .. " -> " .. targetVol .. "dB")
    end
end

-- Fade out all current tracks
local function fadeOutAllTracks(duration)
    duration = duration or CONFIG.fadeDuration
    local trackCount = globalBGMGetTrackCount()
    
    log("[GlobalAudio] Fading out " .. trackCount .. " tracks")
    
    for i = 0, trackCount - 1 do
        globalBGMFade(i, -80, duration)
    end
end

-- Fade out a specific track
local function fadeOutTrack(index, duration)
    duration = duration or CONFIG.fadeDuration
    globalBGMFade(index, -80, duration)
end

-- Fade in a specific track to its default volume
local function fadeInTrack(sceneType, index, duration)
    duration = duration or CONFIG.fadeDuration
    local config = CONFIG[sceneType]
    if config and config.defaultVolumes[index] then
        globalBGMFade(index, config.defaultVolumes[index], duration)
    else
        globalBGMFade(index, 0, duration)
    end
end

-- ==================== SCENE TRANSITIONS ====================

-- Called when scene starts
local function onSceneStart()
    currentScene = detectCurrentScene()
    log("[GlobalAudio] Scene started: " .. currentScene)
    log("[GlobalAudio] Previous scene: " .. tostring(previousScene))
    log("[GlobalAudio] Track count: " .. globalBGMGetTrackCount())
    log("[GlobalAudio] Initialized: " .. tostring(globalBGMIsInitialized()))
    
    -- Handle different scene scenarios
    if currentScene == "mainmenu" then
        handleMainMenuStart()
    elseif currentScene == "howtoplay" or currentScene == "howtoplay2" then
        handleHowToPlayStart()
    elseif currentScene == "level1" then
        handleLevel1Start()
    else
        -- Default: just fade in whatever tracks exist
        if globalBGMGetTrackCount() > 0 then
            fadeInSceneTracks(currentScene)
        end
    end
end

function handleMainMenuStart()
    local wasFromLevel1 = (previousScene == "level1")
    local wasFromHowToPlay = (previousScene == "howtoplay" or previousScene == "howtoplay2")
    
    if wasFromLevel1 then
        -- Coming from Level1 - tracks should already be crossfading
        log("[GlobalAudio] Returning from Level1, fading in mainmenu tracks")
        fadeInSceneTracks("mainmenu")
        
    elseif wasFromHowToPlay then
        -- Coming back from howtoplay - fade the layer back in
        log("[GlobalAudio] Returning from HowToPlay, fading layer back in")
        local config = CONFIG.mainmenu
        globalBGMFade(config.LAYER, config.defaultVolumes[config.LAYER], CONFIG.fadeDuration)
        
    else
        -- Fresh start (game launch) - fade in both tracks
        log("[GlobalAudio] Fresh start, fading in all mainmenu tracks")
        fadeInSceneTracks("mainmenu")
    end
end

function handleHowToPlayStart()
    -- HowToPlay scenes don't change audio, just keep whatever is playing
    -- If coming from mainmenu, fade out the layer track
    
    if previousScene == "mainmenu" then
        log("[GlobalAudio] Entering HowToPlay from mainmenu, fading out layer")
        local config = CONFIG.mainmenu
        -- Fade out the layer, keep BGM playing
        globalBGMFade(config.LAYER, -80, CONFIG.fadeDuration)
    else
        log("[GlobalAudio] HowToPlay scene, keeping current audio")
    end
    
    -- Note: howtoplay2 <-> howtoplay transitions don't change audio
end

function handleLevel1Start()
    -- Level1 has different tracks than mainmenu
    -- The sysAudio.cpp should have already crossfaded if tracks are different
    
    log("[GlobalAudio] Level1 started, fading in gameplay tracks")
    
    -- Wait a moment for new tracks to be ready (they start muted)
    -- Then fade in BGM and ambience
    local config = CONFIG.level1
    
    -- Fade in the main tracks
    globalBGMFade(config.BGM, config.defaultVolumes[config.BGM], CONFIG.fadeDuration)
    globalBGMFade(config.AMBIENCE, config.defaultVolumes[config.AMBIENCE], CONFIG.fadeDuration)
    -- Layer stays muted until gameplay triggers it
end

-- ==================== SCENE CHANGE HANDLERS ====================
-- Call these before triggering scene change

function prepareTransitionToScene(targetScene)
    log("[GlobalAudio] Preparing transition to: " .. targetScene)
    previousScene = currentScene
    isTransitioning = true
    
    if targetScene == "level1" then
        -- Fade out current audio before Level1 loads
        fadeOutAllTracks(CONFIG.crossfadeDuration)
        log("[GlobalAudio] Fading out before Level1 load")
        
    elseif targetScene == "mainmenu" and currentScene == "level1" then
        -- Fade out Level1 audio
        fadeOutAllTracks(CONFIG.crossfadeDuration)
        -- Clear so mainmenu can start fresh
        globalBGMClear()
        log("[GlobalAudio] Fading out Level1 audio for mainmenu")
        
    elseif targetScene == "howtoplay" or targetScene == "howtoplay2" then
        -- Just fade layer, keep BGM
        if currentScene == "mainmenu" then
            local config = CONFIG.mainmenu
            globalBGMFade(config.LAYER, -80, CONFIG.fadeDuration)
        end
    end
end

-- ==================== QUIT HANDLING ====================

function prepareQuit()
    log("[GlobalAudio] Preparing to quit, fading all audio")
    globalBGMFadeAllAndQuit(CONFIG.quitFadeDuration)
    
    -- Schedule the actual quit after fade
    setTimeout(function()
        log("[GlobalAudio] Fade complete, quitting")
        quitApplication()
    end, CONFIG.quitFadeDuration + 0.5)
end

-- ==================== INITIALIZATION ====================

-- Register the start handler
registerStart(function()
    log("[GlobalAudio] === GlobalAudioController initialized ===")
    onSceneStart()
end)

-- Optional: Update loop if we need ongoing checks
-- registerUpdate(function(dt)
--     -- Could add dynamic audio triggers here
-- end)

-- ==================== PUBLIC API FOR OTHER SCRIPTS ====================
-- These functions can be called from other Lua scripts

-- Expose functions globally
_G.GlobalAudio = {
    fadeInTrack = fadeInTrack,
    fadeOutTrack = fadeOutTrack,
    fadeOutAll = fadeOutAllTracks,
    fadeInScene = fadeInSceneTracks,
    prepareTransition = prepareTransitionToScene,
    prepareQuit = prepareQuit,
    getCurrentScene = function() return currentScene end,
    getPreviousScene = function() return previousScene end
}

log("[GlobalAudio] GlobalAudio API registered")
