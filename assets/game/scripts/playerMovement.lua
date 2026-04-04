-- player movement script
local moveLeft = false
local moveRight = false
local moveUp = false
local moveDown = false
local jumpPressed = false
local wasMoving = false

-- Animation
local ANIM_IDLE = "Frog_Idle" 
local ANIM_WALK = "Frog_Hopping"
local ANIM_JUMP = "Frog_Jump"
local ANIM_FLIGHT = "Frog_Flight"
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

-- shared input API for other UIActions.lua
_G.PlayerInput = _G.PlayerInput or {}
local PlayerInput = _G.PlayerInput

function PlayerInput.requestJump()
    if IsGamePaused() then return end
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
    if IsGamePaused() then 
        joystickDirX = 0.0
        joystickDirY = 0.0
        return 
    end
    
    if dirX ~= nil and dirY ~= nil then
        joystickDirX = dirX
        joystickDirY = dirY
    end
end

-- ==================== DYNAMIC KEY DISPATCH ====================
-- Instead of hardcoding keys, we register handlers for ALL rebindable keys
-- and check _G.KeyBindings at input time to determine which action to trigger.

local KB = _G.KeyBindings
if not KB then
    -- Fallback if KeyBindings.lua hasn't loaded yet
    _G.KeyBindings = {
        forward = "W", left = "A", right = "D", backward = "S",
        jump = "SPACE", hide = "E",
    }
    KB = _G.KeyBindings
end

-- List of all keys we need to listen to (the valid rebindable set)
local ALL_LISTEN_KEYS = {
    "A","B","C","D","E","F","G","H","I","J","K","L","M",
    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    "0","1","2","3","4","5","6","7","8","9",
    "SPACE","TAB","LSHIFT","RSHIFT"
}

for _, keyName in ipairs(ALL_LISTEN_KEYS) do
    registerKeyDown(keyName, function()
        if IsGamePaused() then return end
        if KB.forward  == keyName then moveUp    = true end
        if KB.backward == keyName then moveDown  = true end
        if KB.left     == keyName then moveLeft  = true end
        if KB.right    == keyName then moveRight = true end
        if KB.jump     == keyName then jumpPressed = true end
    end)

    registerKeyUp(keyName, function()
        if KB.forward  == keyName then moveUp    = false end
        if KB.backward == keyName then moveDown  = false end
        if KB.left     == keyName then moveLeft  = false end
        if KB.right    == keyName then moveRight = false end
    end)
end

local speed = 3
local pipeSpeedFactor = 0.50  -- multiplier applied to speed while inside a pipe
local jumpSpeed = 5
local isGrounded = true
local wasGrounded = true
local groundY = nil

local I = nil
local baseRx, baseRy, baseRz = getRotation(entityId)
local currentYaw = baseRy or 0.0
local playerStateInited = false

local landingTimer = 0.0 
local idleTimer = 0.0
local hopTimer = 0.0
local idleInterval = 5.0 + math.random() * 2.0  -- Random 5-8 seconds
local S = nil

local maxGroundCheckDist = 0.1
local jumpCooldown = 0.0
local FOOT_PARTICLE_OFFSET_Y = 0.03
local FOOT_PARTICLE_OFFSET_Z = -0.18
local FOOT_PARTICLE_JITTER_X = 0.11
local FOOT_PARTICLE_JITTER_Y = 0.04
local FOOT_PARTICLE_JITTER_Z = 0.08
local FOOT_PARTICLE_BURSTS = 2
local WALK_PARTICLE_COUNT = 5
local WALK_LOOP_PARTICLE_COUNT = 3
local JUMP_PARTICLE_COUNT = 6

local function randomRange(minVal, maxVal)
    return minVal + (maxVal - minVal) * math.random()
end

local function emitFootPebbles(id, totalCount)
    local burstCount = math.min(FOOT_PARTICLE_BURSTS, totalCount)
    if burstCount <= 0 then return end

    local baseCount = math.floor(totalCount / burstCount)
    local remainder = totalCount % burstCount

    for i = 1, burstCount do
        local burstParticles = baseCount
        if i <= remainder then
            burstParticles = burstParticles + 1
        end

        if burstParticles > 0 then
            local offsetX = randomRange(-FOOT_PARTICLE_JITTER_X, FOOT_PARTICLE_JITTER_X)
            local offsetY = FOOT_PARTICLE_OFFSET_Y + randomRange(0.0, FOOT_PARTICLE_JITTER_Y)
            local offsetZ = FOOT_PARTICLE_OFFSET_Z + randomRange(-FOOT_PARTICLE_JITTER_Z, FOOT_PARTICLE_JITTER_Z)
            spawnParticlesAtOffset(id, burstParticles, offsetX, offsetY, offsetZ)
        end
    end
end

-- Player SFX file paths
local SFX_PLAYER_HOPS = {
    "game/audio/sfx/player/frog footstep/Frog_Footstep_01.wav",
    "game/audio/sfx/player/frog footstep/Frog_Footstep_02.wav",
    "game/audio/sfx/player/frog footstep/Frog_Footstep_03.wav",
    "game/audio/sfx/player/frog footstep/Frog_Footstep_04.wav",
    "game/audio/sfx/player/frog footstep/Frog_Footstep_05.wav",
    "game/audio/sfx/player/frog footstep/Frog_Footstep_06.wav",
    "game/audio/sfx/player/frog footstep/Frog_Footstep_07.wav",
    "game/audio/sfx/player/frog footstep/Frog_Footstep_08.wav",
    "game/audio/sfx/player/frog footstep/Frog_Footstep_09.wav"
}

-- Old hop sound, maybe use for jumps only
--local SFX_PLAYER_HOPS = {
    --"game/audio/sfx/player/frog hop/Player_Hop_01.wav",
    --"game/audio/sfx/player/frog hop/Player_Hop_02.wav",
    --"game/audio/sfx/player/frog hop/Player_Hop_03.wav",
    --"game/audio/sfx/player/frog hop/Player_Hop_04.wav",
    --"game/audio/sfx/player/frog hop/Player_Hop_05.wav",
    --"game/audio/sfx/player/frog hop/Player_Hop_06.wav",
    --"game/audio/sfx/player/frog hop/Player_Hop_07.wav"
--}

local SFX_PLAYER_IDLES = {
    "game/audio/sfx/player/frog croak/Player_VO_Idle_01.wav",
    "game/audio/sfx/player/frog croak/Player_VO_Idle_02.wav",
    "game/audio/sfx/player/frog croak/Player_VO_Idle_03.wav",
    "game/audio/sfx/player/frog croak/Player_VO_Idle_04.wav",
    "game/audio/sfx/player/frog croak/Player_VO_Idle_05.wav"
}

-- SFX Volumes modifier
local VOL_PLAYER_HOP = 1.0
local VOL_PLAYER_IDLE = 0.7

registerUpdate(function(dt)
    -- EARLY EXIT: If camera is panning, freeze player completely
    if _G.CameraPan and _G.CameraPan.active then
        if walkingSoundPlaying and audioStop then
            audioStop(entityId)
            walkingSoundPlaying = false
        end
        joystickDirX = 0.0
        joystickDirY = 0.0
        local _, curr_vy, _ = getVelocity(entityId)
        setVelocity(entityId, 0.0, curr_vy, 0.0)
        particleSystemStop(entityId)
        return
    end

    -- EARLY EXIT: If game is paused, freeze player completely
    if IsGamePaused() then
        if walkingSoundPlaying and audioStop then
            audioStop(entityId)
            walkingSoundPlaying = false
        end
        joystickDirX = 0.0
        joystickDirY = 0.0
        particleSystemStop(entityId)
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
        particleSystemStop(id)
        return
    end

    -- pipe traversal: constrained 1D movement along the pipe axis
    if S and S.isInPipe and S.isInPipe() then
        jumpPressed = false

        -- WASD or joystick (W / joystick up is toward exit)
        local fwd = 0.0
        if moveUp   then fwd = fwd + 1.0 end
        if moveDown then fwd = fwd - 1.0 end
        fwd = fwd + joystickDirY
        fwd = math.max(-1.0, math.min(1.0, fwd))

        local px, py, pz = getPosition(id)
        local ep = S.pipeEntrancePos

        -- project player's current position onto the pipe axis
        local toCurX = px - ep.x
        local toCurY = py - ep.y
        local toCurZ = pz - ep.z
        local currentT = toCurX * S.pipeDirX + toCurY * S.pipeDirY + toCurZ * S.pipeDirZ

        -- advance and clamp within [0, pipeLen]
        local newT = currentT + fwd * speed * pipeSpeedFactor * dt

        newT = math.max(0.0, math.min(S.pipeLen, newT))
        setPosition(id,
            ep.x + S.pipeDirX * newT,
            ep.y + S.pipeDirY * newT,
            ep.z + S.pipeDirZ * newT)
        setVelocity(id, 0.0, 0.0, 0.0)

        -- face direction of travel
        if fwd ~= 0.0 then
            local sign = fwd > 0.0 and 1.0 or -1.0
            local fdx = S.pipeDirX * sign
            local fdz = S.pipeDirZ * sign
            if fdx*fdx + fdz*fdz > 0.0001 then
                currentYaw = math.atan(fdx, fdz)
            end
        end
        setRotation(id, baseRx, currentYaw, baseRz)
        return
    end

    -- ground check based on physics
    isGrounded = isGrounded_(id, maxGroundCheckDist)

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

    -- Tick jump cooldown
    if jumpCooldown > 0 then
        jumpCooldown = jumpCooldown - dt
    end

    if isMoving then
        Animation.SetSpeed(id, 1.8)
        -- PLAY walking Animation
        if isGrounded then
            PlayAnim(id, ANIM_WALK, 0.1, true) -- on the ground
            Animation.SetLoop(id, true)
        end
        if not isGrounded then
            PlayAnim(id, ANIM_FLIGHT, 0.1, true) -- moving in the sky
            Animation.SetLoop(id, true)
        end
        
        -- Play hop sound when starting to move
        if not wasMoving then
            
            audioPlayRandomSFXFromEntity(SFX_PLAYER_HOPS, id, VOL_PLAYER_HOP)
            emitFootPebbles(id, WALK_PARTICLE_COUNT)
            lastAnimTime = 0.0
        
        -- LOOP: play hop sound at animation cycle loop point
        elseif Animation and Animation.GetTime then
            local t = Animation.GetTime(id)
            
            -- Play audio + burst debris at the start of each walk cycle
            if t < lastAnimTime and t < 0.2 and isGrounded and jumpCooldown <= 0 then
                audioPlayRandomSFXFromEntity(SFX_PLAYER_HOPS, id, VOL_PLAYER_HOP)
                emitFootPebbles(id, WALK_LOOP_PARTICLE_COUNT)
            end
            
            lastAnimTime = t
        end
        
        wasMoving = true

    else
        -- STOPPED
        wasMoving = false
        lastAnimTime = 1000.0
        particleSystemStop(id)

        if not isGrounded then
            -- Airborne but not moving > show flight, not a looping jump
            PlayAnim(id, ANIM_FLIGHT, 0.1, true)
            Animation.SetSpeed(id, 1.0)
            Animation.SetLoop(id, true)

        elseif isGrounded then
            if not wasGrounded then
                -- Just landed: reset idle timer and play landing snap
                landingTimer = 0.3
                idleTimer = 0.0
                idleInterval = 5.0 + math.random() * 2.0
                PlayAnim(id, ANIM_JUMP, 0.1, false)
                Animation.SetSpeed(id, 2)
                Animation.SetLoop(id, false)
                emitFootPebbles(id, JUMP_PARTICLE_COUNT)
            else
                -- Count down landing snap, then do nothing — idleTimer takes over
                if landingTimer > 0 then
                    landingTimer = landingTimer - dt
                elseif currentAnimState == ANIM_WALK then
                    -- Stopped from walking: snap out of walk loop the same way
                    landingTimer = 0.3
                    idleTimer = 0.0
                    idleInterval = 5.0 + math.random() * 2.0
                    PlayAnim(id, ANIM_JUMP, 0.1, false)
                    Animation.SetSpeed(id, 2)
                    Animation.SetLoop(id, false)
                end
            end
        end
    end

    -- Always update at the END of the frame
    wasGrounded = isGrounded
    if not isMoving then
        if landingTimer <= 0 then  -- don't count while landing snap is playing
            idleTimer = idleTimer + dt
        end
        if idleTimer >= idleInterval then
            idleTimer = 0.0
            idleInterval = 2.0 + math.random() * 2
            audioPlayRandomSFXFromEntity(SFX_PLAYER_IDLES, id, VOL_PLAYER_IDLE)
            PlayAnim(id, ANIM_IDLE, 0.2, true)  -- ✅ back here
            Animation.SetSpeed(id, 1.0)
            Animation.SetLoop(id, true)
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

    -- jump, modify vertical vel -> physics handle gravity
    -- local doubleTapJump = (I and I.doubleTapped) or false
    -- if (jumpPressed or doubleTapJump) and isGrounded then
    if jumpPressed and isGrounded then
        curr_vy = jumpSpeed  
        isGrounded = false

        Animation.SetLoop(id, true)
        PlayAnim(id, ANIM_JUMP, 0.05, true)

        -- Play random hop sound for jump
        audioPlayRandomSFXFromEntity(SFX_PLAYER_HOPS, id, VOL_PLAYER_HOP)
        emitFootPebbles(id, JUMP_PARTICLE_COUNT)

        -- Prevent walk-cycle hop from double-playing on landing
        jumpCooldown = 0.35
        lastAnimTime = 0.0
        
        -- consume jump
        jumpPressed = false
    end

    if not isGrounded and math.abs(curr_vy) < 0.5 then
        curr_vx = 0.0
        curr_vz = 0.0
    end

    setRotation(id, baseRx, currentYaw, baseRz)
    setVelocity(id, vx, curr_vy, vz)

    jumpPressed = false
end)
