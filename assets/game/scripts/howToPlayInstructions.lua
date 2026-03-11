local initial = true

registerUpdate(function(dt)

    if initial then
        local isMobile = (isAndroid ~= nil and isAndroid())

        if isMobile then

            local fileName = "game/textures/how to play pg1- mobile ver NEW.ktx"
            local guid = getImageID(fileName)
            
            if guid ~= "" and guid ~= nil then
                print("3. Success! Found: " .. fileName .. " | GUID: " .. guid)
                setTexture(entityId, guid)
            else
                print("2. Asset Error: Could not find " .. fileName)
            end
            
        else
            local fileName = "game/textures/how to play pg1- pc ver NEW.png"
            local guid = getImageID(fileName)
            
            if guid ~= "" and guid ~= nil then
                print("3. Success! Found: " .. fileName .. " | GUID: " .. guid)
                setTexture(entityId, guid)
            else
                print("2. Asset Error: Could not find " .. fileName)
            end
        end

        initial = false
    end

end)