-- player movement script
local moveLeft = false
local moveRight = false
local moveUp = false
local moveDown = false
local jumpPressed = false
local wasMoving = false

-- Animation
local ANIM_IDLE = "Frog_RigAction" 
local ANIM_WALK = "Frog_Jump"
local ANIM_JUMP = "Frog_Jump"
local currentAnimState = "" 
local lastAnimTime =  0.0

-- Helper to switch animation smoothly
local function PlayAnim(id, animName, fadeTime, loop)
    if currentAnimState == animName then return end
    
    -- Check if the C++ binding exists before calling to prevent crash
    if Animation and Animation.CrossFade then
        Animation.CrossFade(id, animName, fadeTime)
        Animation.SetLoop(id, loop)
    end
    
    currentAnimState = animName
end

-- Joystick input (set by UI drag callback)
local joystickDirX = 0.0
local joystickDirY = 0.0

-- -- for UI button (on_click_callback_lua = "JumpButton_OnClick")
-- _G_root.JumpButton_OnClick = function()
--     printLog("[UI] Jump button pressed, setting jumpPressed")
--     if PlayerState and PlayerState.isHidden and PlayerState.isHidden() then
--         return
--     end
--     jumpPressed = true
-- end

-- _G_root.ActionButton_OnClick = function()
--     printLog("[UI] Action button pressed (hide / collect)")
--     if PlayerState and PlayerState.onActionButton then
--         PlayerState.onActionButton()
--     end
-- end

-- shared input API for other UIActions.lua
_G.PlayerInput = _G.PlayerInput or {}
local PlayerInput = _G.PlayerInput

function PlayerInput.requestJump()
    if _G_root.gamePaused then return end
    printLog("[Input] requestJump from UI")

    -- Same guard as before: cannot jump while hidden
    if PlayerState and PlayerState.isHidden and PlayerState.isHidden() then
        return
    end

    jumpPressed = true
end

-- Callback for joystick (on_click_callback_lua = "Joystick_OnDrag")
_G_root.Joystick_OnDrag = function(dirX, dirY)
    -- Don't process joystick when paused
    if _G_root.gamePaused then 
        joystickDirX = 0.0
        joystickDirY = 0.0
        return 
    end
    
    if dirX ~= nil and dirY ~= nil then
        joystickDirX = dirX
        joystickDirY = dirY
    end
end

registerKeyDown("W", function() 
    if not _G_root.gamePaused then moveUp = true end 
end)
registerKeyDown("S", function() 
    if not _G_root.gamePaused then moveDown = true end 
end)
registerKeyDown("A", function() 
    if not _G_root.gamePaused then moveLeft = true end 
end)
registerKeyDown("D", function() 
    if not _G_root.gamePaused then moveRight = true end 
end)
registerKeyDown("SPACE", function() 
    if not _G_root.gamePaused then jumpPressed = true end 
end)

registerKeyUp("W", function() moveUp = false end)
registerKeyUp("S", function() moveDown = false end)
registerKeyUp("A", function() moveLeft = false end)
registerKeyUp("D", function() moveRight = false end)

local speed = 3
local jumpSpeed = 5
local isGrounded = true
local groundY = nil

local I = nil
local baseRx, baseRy, baseRz = getRotation(entityId)
local currentYaw = baseRy or 0.0
local playerStateInited = false

local idleTimer = 0.0
local idleInterval = 5.0
local S = nil

local maxGroundCheckDist = 0.1

registerUpdate(function(dt)
    -- EARLY EXIT: If game is paused, freeze player completely
    if _G_root.gamePaused then
        -- Stop walking audio
        if walkingSoundPlaying and audioStop then
            audioStop(entityId)
            walkingSoundPlaying = false
        end
        
        -- Clear all movement inputs
        joystickDirX = 0.0
        joystickDirY = 0.0
        
        -- Stop all movement
        setVelocity(entityId, 0.0, 0.0, 0.0)
        return
    end
    
    local id = entityId
    _G.PlayerEntity = id

    if not I and _G.Input then
        I = _G.Input
    end

    if PlayerState and PlayerState.init then
        if not PlayerState.player or PlayerState.player ~= entityId then
            PlayerState.init(entityId)
        end
    end

    if not S and _G.PlayerState then
        S = _G.PlayerState
    end

    -- freeze player when game has ended
    if PlayerState and PlayerState.isGameEnded and PlayerState.isGameEnded() then
        -- stop walking audio
        if audioStop then
            audioStop(id)
        end
        setVelocity(id, 0.0, 0.0, 0.0)
        jumpPressed = false
        return
    end

    -- while hiding: stop movement + stop audio 
    if PlayerState and PlayerState.isHidden and PlayerState.isHidden() then
        jumpPressed = false

        if audioStop then
            audioStop(id)
        end
        return
    end

    -- read current transform and vel from physics
    local x, y, z = getPosition(id) 
    if groundY == nil then
        groundY = y
    elseif isGrounded and math.abs(y - groundY) > 0.01 then
        groundY = y 
    end
    local curr_vx, curr_vy, curr_vz = getVelocity(id)

    -- input -> movement
    local dx, dz = 0.0, 0.0

    -- 1. PC: WASD keys
    if moveUp    then dz = dz + 1.0 end
    if moveDown  then dz = dz - 1.0 end
    if moveLeft  then dx = dx + 1.0 end
    if moveRight then dx = dx - 1.0 end

    -- 2. Android: left side of screen controls player movement (DISABLED - using virtual joystick instead)
    --[[
    if getMobileMoveAxes ~= nil then
        local mx, my = getMobileMoveAxes()  
        dx = dx + mx
        dz = dz - my
    end
    --]]

    -- ---------------------------------------------------------
    -- MOVEMENT LOGIC
    -- ---------------------------------------------------------
    -- 3. Virtual joystick input
    dx = dx - joystickDirX
    dz = dz + joystickDirY

    local isMoving = (dx ~= 0.0 or dz ~= 0.0)


    if isMoving then
        -- PLAY walk Animation
        if isGrounded then
            PlayAnim(id, ANIM_WALK, 0.1, true) -- on the ground
            Animation.SetLoop(id, true)
        end

        if not wasMoving then
            if audioPlay then 
                 audioSetLooping(id, false)
                 audioPlay(id) 
            end
            -- Reset lastAnimTime to 0.0 so we don't trigger again immediately
            lastAnimTime = 0.0 
        
        -- LOOP
        elseif Animation and Animation.GetTime then
            local t = Animation.GetTime(id)
            
            -- Play audio at the start of the walk cycle
            if t < lastAnimTime and t < 0.2 then
                 if audioPlay then audioPlay(id) end
            end
            
            lastAnimTime = t
        end
        
        wasMoving = true

    else
        -- STOPPED
        wasMoving = false
        lastAnimTime = 1000.0
        
        -- Make sure animation is not looping
        if isGrounded then Animation.SetLoop(id, false) end
    end

    -- idle sfx: when not moving, in intervals
    if not isMoving then
        idleTimer = idleTimer + dt
        if idleTimer >= idleInterval then
            idleTimer = 0.0
            if S and S.sfxIdle then
                audioPlay(S.sfxIdle)
                Animation.SetLoop(id, true) -- overwrite is grounded animation
                PlayAnim(id, ANIM_IDLE, 0.2, true)
            end
        end
    else
        idleTimer = 0.0
    end


    -- ---------------------------------------------------------
    -- CAMERA LOGIC
    -- ---------------------------------------------------------
    if _G.CameraState ~= nil and _G.CameraState.yaw ~= nil then
        local cy = _G.CameraState.yaw
        local sinY = math.sin(cy)
        local cosY = math.cos(cy)

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

        x = x + dx * speed * dt
        z = z + dz * speed * dt

        local newYaw = math.atan(dx, dz)   

        local diff = newYaw - currentYaw
        if diff > math.pi then
            newYaw = newYaw - 2*math.pi
        elseif diff < -math.pi then
            newYaw = newYaw + 2*math.pi
        end

        currentYaw = newYaw
    end

    -- ---------------------------------------------------------
    -- JUMP LOGIC
    -- ---------------------------------------------------------
    -- ground check based on physics
    isGrounded = isGrounded_(id, maxGroundCheckDist)

    -- jump, modify vertical vel -> physics handle gravity
    -- local doubleTapJump = (I and I.doubleTapped) or false
    -- if (jumpPressed or doubleTapJump) and isGrounded then
    if jumpPressed and isGrounded then
        curr_vy = jumpSpeed  
        isGrounded = false

        Animation.SetLoop(id, true)
        PlayAnim(id, ANIM_JUMP, 0.05, true)

        -- jump sfx
        if S and S.sfxJump then
            audioPlay(S.sfxJump)
        end
        
        -- consume jump
        jumpPressed = false
    end

    setRotation(id, baseRx, currentYaw, baseRz)
    setVelocity(id, vx, curr_vy, vz)

    jumpPressed = false
end)
