-- 1. DEFINE THESE OUTSIDE THE FUNCTION
if currentFrame == nil then
    currentFrame = 1
end
local maxFrames = 7
local G = _G_root

G.Level1SceneName   = G.Level1SceneName  or "game/scenes/Level1.scn"

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