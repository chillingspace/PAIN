-- cutScene2Logic.lua (Cutscene 2)
-- 1. DEFINE THESE OUTSIDE THE FUNCTION
if currentFrame == nil then
    currentFrame = 1
end
local maxFrames = 7
local G = _G_root

local frameDuration = 2.0
local timer = 0.0

G.Level1SceneName   = G.Level1SceneName  or "game/scenes/Level1.scn"

-- ==================== CUTSCENE MASTER AUDIO ====================
-- Single master audio file plays for the entire cutscene duration (no loop)
local CS2_MASTER_AUDIO = "game/audio/sfx/cutscenes/cs2 master audio.wav"

if not _G.cs2_sfx_init then
    _G.cs2_sfx_init = true
    if audioPlaySFX then
        local chId = audioPlaySFX(CS2_MASTER_AUDIO, 0.0)
        if chId and chId >= 0 then
            _G.SFXChannels = _G.SFXChannels or {}
            _G.SFXChannels[chId] = true
        end
        print("[Cutscene2] Playing master audio: " .. CS2_MASTER_AUDIO)
    end
end

-- ==================== OLD PER-FRAME SFX (commented out) ====================
--[[ OLD PER-FRAME SFX SYSTEM - kept for potential revert
local SFX_PATH = "game/audio/sfx/cutscenes/"
local FRAME_SFX = {
    [1] = { SFX_PATH .. "fixing robot.wav" },
    -- [2] no SFX
    [3] = { SFX_PATH .. "robot lights turn on cs2.wav" },
    [4] = { SFX_PATH .. "robot light getting stronger cs2.wav" },
    [5] = { SFX_PATH .. "robot lights shut off cs2.wav" },
    -- [6] no SFX
    [7] = { SFX_PATH .. "enemy walking cs2.wav" },
}

local activeSfxChannels = {}

local function stopPreviousSFX()
    if audioStopChannel then
        for _, chId in ipairs(activeSfxChannels) do
            if chId >= 0 then audioStopChannel(chId) end
        end
    end
    activeSfxChannels = {}
end

local function playCutsceneSFX(frame)
    stopPreviousSFX()
    local sfxList = FRAME_SFX[frame]
    if not sfxList or not audioPlaySFX then return end
    for _, sfxFile in ipairs(sfxList) do
        local chId = audioPlaySFX(sfxFile, 0.0)
        if chId and chId >= 0 then
            table.insert(activeSfxChannels, chId)
        end
        print("[Cutscene2] Playing SFX for frame " .. frame .. ": " .. sfxFile)
    end
end

if not _G.cs2_sfx_init then
    _G.cs2_sfx_init = true
    playCutsceneSFX(1)
end
--]]

registerUpdate(function(dt)
    -- Check for mouse click
    if _G_root.IsGamePaused() then
        return
    end

    timer = timer + dt

    if timer >= frameDuration then
    --if wasMousePressed(0) then

        timer = 0
        print("Next Panel")
        
        -- 2. Increment the frame
        currentFrame = currentFrame + 1

        local isMobile = (isAndroid ~= nil and isAndroid())
        
        -- 3. Check if we finished the cutscene
        if currentFrame > maxFrames then 
            print("[Cutscene] Last frame reached. Changing scene with audio fade...")
            
            local nextScene = G.Level1SceneName 
            
            if isMobile then
                hideCursor(true)
            end
            
            -- Use GlobalAudio's fade system for proper audio transition
            if _G.GlobalAudio and _G.GlobalAudio.changeSceneWithFade then
                _G.GlobalAudio.changeSceneWithFade(nextScene)
            elseif changeScene then
                -- Fallback to direct change if GlobalAudio not available
                changeScene(nextScene)
            else
                print("[Cutscene] Error: No scene change function available")
            end
            return
        end
        
        -- Play SFX for the new frame (OLD per-frame system, commented out)
        -- playCutsceneSFX(currentFrame)
        
        -- Determine file path based on platform
        local fileName = ""
        if isMobile then
            -- Android: use KTX format (compressed texture format)
            fileName = "game/textures/cs2-" .. currentFrame .. ".ktx"
        else
            -- Windows: use PNG format
            fileName = "game/textures/cs2-" .. currentFrame .. ".png"
        end
        
        local guid = getImageID(fileName)
        
        if guid ~= "" and guid ~= nil then
            print("3. Success! Found: " .. fileName .. " | GUID: " .. guid)
            setTexture(entityId, guid)
        else
            print("2. Asset Error: Could not find " .. fileName)
        end
    end
end)