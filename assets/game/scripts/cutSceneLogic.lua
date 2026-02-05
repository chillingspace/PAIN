-- 1. DEFINE THESE OUTSIDE THE FUNCTION
if currentFrame == nil then
    currentFrame = 1
end
local maxFrames = 15
local G = _G_root

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
            print("[Cutscene] Last frame reached. Changing scene...")
            
            if changeScene then
                local nextScene = G.TutorialSceneName 
                
                if isMobile then
                    hideCursor(true)
                end
                
                changeScene(nextScene)
            else
                print("[Cutscene] Error: changeScene function not found")
            end
            return
        end
        
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