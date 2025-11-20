
-- player movement script

local moveLeft = false
local moveRight = false
local moveUp = false
local moveDown = false

registerKeyDown("KEY_U", function() moveUp = true end)
registerKeyDown("KEY_D", function() moveDown = true end)
registerKeyDown("KEY_L", function() moveLeft = true end)
registerKeyDown("KEY_R", function() moveRight = true end)

registerKeyUp("KEY_U", function() moveUp = false end)
registerKeyUp("KEY_D", function() moveDown = false end)
registerKeyUp("KEY_L", function() moveLeft = false end)
registerKeyUp("KEY_R", function() moveRight = false end)

local speed = 4.0

-- grab initial rotation 
local baseRx, baseRy, baseRz = getRotation(entityId)
local currentYaw = baseRy or 0.0

local playerStateInited = false

registerUpdate(function(dt)
    if not playerStateInited then
        if PlayerState and PlayerState.init then
            PlayerState.init(entityId)
        end
        playerStateInited = true
    end

    local x, y, z = getPosition(entityId)
    local dx, dz = 0.0, 0.0

    if moveUp    then dz = dz - 1.0 end
    if moveDown  then dz = dz + 1.0 end
    if moveLeft  then dx = dx - 1.0 end
    if moveRight then dx = dx + 1.0 end

    local len2 = dx*dx + dz*dz
    if len2 > 0.0001 then
        local invLen = 1.0 / math.sqrt(len2)
        dx = dx * invLen
        dz = dz * invLen

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

        setRotation(entityId, baseRx, currentYaw, baseRz)
    end


    setPosition(entityId, x, y, z)
end)