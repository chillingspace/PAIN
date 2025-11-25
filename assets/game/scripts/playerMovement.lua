-- player movement script

local moveLeft = false
local moveRight = false
local moveUp = false
local moveDown = false

local walkingSoundPlaying = false

-- Input Registration with Debug Logs
registerKeyDown("KEY_U", function() 
    --print("DEBUG: KEY_U Pressed") 
    moveUp = true 
end)
registerKeyDown("KEY_D", function() 
    --print("DEBUG: KEY_D Pressed")
    moveDown = true 
end)
registerKeyDown("KEY_L", function() 
    --print("DEBUG: KEY_L Pressed")
    moveLeft = true 
end)
registerKeyDown("KEY_R", function() 
    --print("DEBUG: KEY_R Pressed")
    moveRight = true 
end)

registerKeyUp("KEY_U", function() 
    --print("DEBUG: KEY_U Released")
    moveUp = false 
end)
registerKeyUp("KEY_D", function() moveDown = false end)
registerKeyUp("KEY_L", function() moveLeft = false end)
registerKeyUp("KEY_R", function() moveRight = false end)

local speed = 4.0

-- grab initial rotation 
local baseRx, baseRy, baseRz = getRotation(entityId)
local currentYaw = baseRy or 0.0
local playerStateInited = false

registerUpdate(function(dt)
    local id = entityId -- the entity script is attached to

    -- Verify if the script is ticking at all
    --print("DEBUG: Update ticking for Entity ID: " .. tostring(id))

    if not playerStateInited then
        if PlayerState and PlayerState.init then
            PlayerState.init(entityId)
        end
        playerStateInited = true
    end

    -- while hiding: stop movement + stop audio 
    if PlayerState and PlayerState.isHidden and PlayerState.isHidden() then
        if walkingSoundPlaying and audioStop then
            audioStop(id)
            walkingSoundPlaying = false
        end
        return
    end

    local x, y, z = getPosition(id)
    local dx, dz = 0.0, 0.0

    if moveUp    then dz = dz - 1.0 end
    if moveDown  then dz = dz + 1.0 end
    if moveLeft  then dx = dx - 1.0 end
    if moveRight then dx = dx + 1.0 end

    local isMoving = (dx ~= 0.0 or dz ~= 0.0)

    if isMoving then
        -- Log only when trying to move to avoid console spam
        print("DEBUG: Movement detected. dx: " .. dx .. " dz: " .. dz) 
        
        if not walkingSoundPlaying and audioPlay then
            -- optional: ensure it loops
            if audioSetLooping then audioSetLooping(id, true) end
            audioPlay(id)
            walkingSoundPlaying = true
        end
    else
        if walkingSoundPlaying and audioStop then
            audioStop(id)
            walkingSoundPlaying = false
        end
    end

    local len2 = dx*dx + dz*dz
    local vx, vy, vz = 0.0, 0.0, 0.0
    
    if len2 > 0.0001 then
        local invLen = 1.0 / math.sqrt(len2)
        dx = dx * invLen
        dz = dz * invLen

        vx = dx * speed
        vz = dz * speed

        -- move in that direction
        x = x + dx * speed * dt
        z = z + dz * speed * dt

        local newYaw = math.atan(dx, dz)   

        -- unwrap to avoid jumps across +pi / -pi
        local diff = newYaw - currentYaw
        if diff > math.pi then
            newYaw = newYaw - 2*math.pi
        elseif diff < -math.pi then
            newYaw = newYaw + 2*math.pi
        end

        currentYaw = newYaw
    end

    local curr_vx, curr_vy, curr_vz = getVelocity(id)

    setVelocity(id, vx, curr_vy, vz)
end)