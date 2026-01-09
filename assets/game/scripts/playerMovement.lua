
-- player movement script

local moveLeft = false
local moveRight = false
local moveUp = false
local moveDown = false
local jumpPressed = false
local walkingSoundPlaying = false

-- for UI button (on_click_callback_lua = "JumpButton_OnClick")
_G_root.JumpButton_OnClick = function()
    printLog("[UI] Jump button pressed, setting jumpPressed")
    if PlayerState and PlayerState.isHidden and PlayerState.isHidden() then
        return
    end
    jumpPressed = true
end

_G_root.ActionButton_OnClick = function()
    printLog("[UI] Action button pressed (hide / collect)")
    if PlayerState and PlayerState.onActionButton then
        PlayerState.onActionButton()
    end
end

registerKeyDown("W", function() moveUp = true end)
registerKeyDown("S", function() moveDown = true end)
registerKeyDown("A", function() moveLeft = true end)
registerKeyDown("D", function() moveRight = true end)
registerKeyDown("SPACE", function() jumpPressed = true end)
registerKeyUp("W", function() moveUp = false end)
registerKeyUp("S", function() moveDown = false end)
registerKeyUp("A", function() moveLeft = false end)
registerKeyUp("D", function() moveRight = false end)

local speed = 0.8
local jumpSpeed = 2
local isGrounded = true
local groundY = nil -- will be set from initial position

local I = nil -- will be hooked to _G.Input once PlayerState has created it

-- grab initial rotation 
local baseRx, baseRy, baseRz = getRotation(entityId)
local currentYaw = baseRy or 0.0
local playerStateInited = false

local idleTimer     = 0.0
local idleInterval  = 5.0   -- seconds between idle sounds when not moving
local S = nil -- will grab _G.PlayerState

-- For ground check
local maxGroundCheckDist = 0.25

registerUpdate(function(dt)
    local id = entityId -- the entity script is attached to

    -- make sure see Input even if PlayerState loaded later
    if not I and _G.Input then
        I = _G.Input
    end

    if not playerStateInited then
        if PlayerState and PlayerState.init then
            PlayerState.init(entityId)
        end
        playerStateInited = true
    end

        if not S and _G.PlayerState then
        S = _G.PlayerState
    end

    -- while hiding: stop movement + stop audio 
    if PlayerState and PlayerState.isHidden and PlayerState.isHidden() then
        -- clear any pending jump inputs so they dont fire after unhide
        -- if I then
        --     I.doubleTapped = false
        --     I.tapCount = 0     
        --     I.tapTimer = 0.0
        -- end
        jumpPressed = false

        if walkingSoundPlaying and audioStop then
            audioStop(id)
            walkingSoundPlaying = false
        end
        return
    end

    -- read current transform and vel from physics
    local x, y, z = getPosition(id) 
    if groundY == nil then
        groundY = y
    elseif isGrounded and math.abs(y - groundY) > 0.01 then -- if player gets teleported to a new floor/checkpoint
        groundY = y 
    end
    local curr_vx, curr_vy, curr_vz = getVelocity(id)

    -- input -> movement
    local dx, dz = 0.0, 0.0

    -- 1. PC: Arrow keys (KEY_U/D/L/R)
    if moveUp    then dz = dz + 1.0 end
    if moveDown  then dz = dz - 1.0 end
    if moveLeft  then dx = dx + 1.0 end
    if moveRight then dx = dx - 1.0 end

    -- 2. Android: left side of screen controls player movement
    if getMobileMoveAxes ~= nil then
        local mx, my = getMobileMoveAxes()  
        dx = dx + mx
        dz = dz - my
    end

    local isMoving = (dx ~= 0.0 or dz ~= 0.0)

    if isMoving then
        if not walkingSoundPlaying and audioPlay then
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

        -- idle sfx: when not moving, in intervals
        if not isMoving then
            idleTimer = idleTimer + dt
            if idleTimer >= idleInterval then
                idleTimer = 0.0
                if S and S.sfxIdle then
                    audioPlay(S.sfxIdle)
                end
            end
        else
            idleTimer = 0.0
        end


    -- Make movement relative to camera yaw, if available.
    -- This means "push up" always moves in front of the camera.
    if _G.CameraState ~= nil and _G.CameraState.yaw ~= nil then
        local cy = _G.CameraState.yaw
        local sinY = math.sin(cy)
        local cosY = math.cos(cy)

        -- dx, dz are in camera-local space.
        -- Convert to world space using camera's right and forward:
        -- right  = ( cosY, 0, -sinY )
        -- forward= ( sinY, 0,  cosY )
        local wx =  cosY * dx + sinY * dz
        local wz = -sinY * dx + cosY * dz

        dx, dz = wx, wz
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

    -- ground check based on physics
    local groundedEpsPos = 0.05
    local groundedEpsVel = 0.1
    isGrounded = isGrounded_(id, maxGroundCheckDist)

    -- jump, modify vertical vel -> physics handle gravity
    --local doubleTapJump = (I and I.doubleTapped) or false
    --if (jumpPressed or doubleTapJump) and isGrounded then
    if jumpPressed and isGrounded then
        curr_vy = jumpSpeed  
        isGrounded = false

        -- jump sfx
        if S and S.sfxJump then
            audioPlay(S.sfxJump)
        end

        -- consume jump
        jumpPressed = false
        -- if I then
        --     I.doubleTapped = false
        -- end
    end

    -- apply rotation and phy velocity
    setRotation(id, baseRx, currentYaw, baseRz)
    setVelocity(id, vx, curr_vy, vz)

    jumpPressed = false
end)