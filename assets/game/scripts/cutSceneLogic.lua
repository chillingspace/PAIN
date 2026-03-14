-- cutSceneLogic.lua (Cutscene 1)
-- 1. DEFINE THESE OUTSIDE THE FUNCTION
if currentFrame == nil then
    currentFrame = 1
end
local maxFrames = 15
local G = _G_root

-- ==================== CUTSCENE SFX ====================
local SFX_PATH = "game/audio/sfx/cutscenes/"
local FRAME_SFX = {
    [1]  = { SFX_PATH .. "frog + robot footsteps.wav" },
    [2]  = { SFX_PATH .. "frog notice.wav" },
    -- [3]  no SFX
    [4]  = { SFX_PATH .. "sparkle sfx.wav" },
    -- [5]  no SFX
    [6]  = { SFX_PATH .. "loud commotion.wav" },
    [7]  = { SFX_PATH .. "cutscene1 enemy surround robot.wav" },
    [8]  = { SFX_PATH .. "enemy alert state - triple.wav" },
    [9]  = { SFX_PATH .. "enemy alert state - triple.wav" },
    [10] = { SFX_PATH .. "throw frog cs1.wav" },
    [11] = { SFX_PATH .. "fall into pile cutscene1.wav" },
    -- [12] no SFX
    [13] = { SFX_PATH .. "frog notice.wav" },
    -- [14] no SFX
    -- [15] no SFX
}

local function playCutsceneSFX(frame)
    local sfxList = FRAME_SFX[frame]
    if not sfxList or not audioPlaySFX then return end
    for _, sfxFile in ipairs(sfxList) do
        audioPlaySFX(sfxFile, 0.0)
        print("[Cutscene1] Playing SFX for frame " .. frame .. ": " .. sfxFile)
    end
end

-- Play SFX for the initial frame (frame 1 is already displayed when scene loads)
playCutsceneSFX(1)

registerUpdate(function(dt)
    -- Check for mouse click
    if _G_root.IsGamePaused() then
        return
    end

    if wasMousePressed(0) then
        print("1. Mouse Click Detected!")
        
        -- 2. Increment the frame
        currentFrame = currentFrame + 1

        local isMobile = (isAndroid ~= nil and isAndroid())
        
        -- 3. Check if we finished the cutscene
        if currentFrame > maxFrames then 
            print("[Cutscene] Last frame reached. Changing scene with audio fade...")
            
            local nextScene = G.TutorialSceneName 
            
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
        
        -- Play SFX for the new frame
        playCutsceneSFX(currentFrame)
        
        -- Determine file path based on platform
        local fileName = ""
        if isMobile then
            -- Android: use KTX format (compressed texture format)
            fileName = "game/textures/c" .. currentFrame .. ".ktx"
        else
            -- Windows: use PNG format
            fileName = "game/textures/c" .. currentFrame .. ".png"
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