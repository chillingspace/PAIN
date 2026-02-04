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
]]

-- ==================== CONFIGURATION ====================
local CONFIG = {
    -- Fade durations (seconds)
    fadeDuration = 2.0,
    crossfadeDuration = 2.5,
    quitFadeDuration = 2.0,
    
    -- Default volumes for different scenarios
    defaultVolume = 0.0,   -- 0 dB = full volume
    mutedVolume = -80.0    -- -80 dB = muted
}

-- ==================== SCENE DETECTION ====================
local function detectCurrentScene()
    -- Only use getCurrentSceneName - no fallback
    if getCurrentSceneName then
        local sceneName = getCurrentSceneName()
        if sceneName then
            local lowerName = string.lower(sceneName)
            
            if string.find(lowerName, "mainmenu") then
                return "mainmenu"
            elseif string.find(lowerName, "howtoplay2") then
                return "howtoplay2"
            elseif string.find(lowerName, "howtoplay") then
                return "howtoplay"
            elseif string.find(lowerName, "level1") then
                return "level1"
            elseif string.find(lowerName, "level") then
                return "level"
            end
        end
    end
    
    return "unknown"
end

-- ==================== INITIALIZATION ====================
local currentScene = nil

local function onSceneStart()
    currentScene = detectCurrentScene()
    
    -- Minimal debug: single line showing key info
    log("[GlobalAudio] Scene:" .. currentScene .. " Tracks:" .. tostring(globalBGMGetTrackCount()) .. " Init:" .. tostring(globalBGMIsInitialized()))
    
    -- Fade in all tracks to their default volume
    local trackCount = globalBGMGetTrackCount()
    for i = 0, trackCount - 1 do
        globalBGMFade(i, CONFIG.defaultVolume, CONFIG.fadeDuration)
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
_G.GlobalAudio.getCurrentScene = function() return currentScene end

-- ==================== REGISTER HANDLERS ====================
registerStart(function()
    onSceneStart()
end)
