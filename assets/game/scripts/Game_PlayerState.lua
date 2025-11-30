
-- Game_PlayerState.lua

-- Shared game state for player lives, checkpoints, and death logic
-- attach this script to sth always present, 
-- or just somehow make sure its always loaded so other scripts can call PlayerState.onCaught(player)

-- global so other scripts can use
_G.PlayerState = {
    lives = 3,
    startPos = nil,
    checkpointPos = nil,
    checkpointEntity = nil,
    respawnCooldown = 0.0, -- seconds of invincibility
    pendingRespawn = nil,

    player = nil, -- cache the player entity
    carriedLetter = nil, -- entity id of letter currently on back
    pickupRadius = 2.0,  -- how close player must be to pick up letter
    deliveryRadius  = 4.0, --  to drop off at collection point
    lettersDelivered = 0,  -- letters delivered so far
    lettersToWin = 3,  -- how many letters needed to win
    carriedOffset = {   -- offset of letter on the player's back
        x = 0.0,
        y = 1.2,
        z = -0.3
    },

    hidden = false, -- is player hiding
    hiddenIn = nil, -- which spot
    hideRadius = 4.5,  -- how close to hide
    playerBaseScale = nil,   -- original scale of player
    letterBaseScale = nil,
    hideScaleFactor = 0.1,   -- how small when hiding 

     -- SFX entities
    sfxHideIn  = nil,
    sfxHideOut = nil,
    sfxRespawn = nil,
    sfxIdle    = nil,
    sfxJump    = nil,
    sfxDrop    = nil,

    _keysRegistered = false
}

_G.Input = _G.Input or {}
local I = _G.Input
I.tapCount = I.tapCount or 0
I.tapTimer = I.tapTimer or 0.0
I.doubleTapped = I.doubleTapped or false
I.doubleTapThreshold = I.doubleTapThreshold or 0.25

local S = _G.PlayerState
local collectPressed = false
local hidePressed = false

-- guard so keys arent registered twice if script reloads
if not S._keysRegistered then
    registerKeyDown("C", function() collectPressed = true end)
    registerKeyUp("C", function() collectPressed = false end)
    registerKeyDown("H", function() hidePressed = true end)
    registerKeyUp("H",   function() hidePressed = false end)
    
    -- mouse click and press (android) treated as a generic action
    if registerOnClick then
        registerOnClick(function()
            I.tapCount = I.tapCount + 1 -- record that a tap happened, timing handled in update
            if I.tapCount == 1 then -- reset timer so we measure from the first tap in a cluster
                I.tapTimer = 0.0
            end
        end)
    end

    S._keysRegistered = true
end

-- called once when we first find the player
function S.init(player)
    if not S.player then
        S.player = player
        -- log("[PlayerState] S.player set to", tostring(player))
    end

    if not S.startPos then
        local x, y, z = getPosition(player)
        S.startPos = { x = x, y = y, z = z }
        S.checkpointPos = { x = x, y = y, z = z }
        -- log("[PlayerState] Init at start position:", x, y, z)
    end

    -- cache the original scale 
    if not S.playerBaseScale then
        local sx, sy, sz = getScale(player)
        S.playerBaseScale = { x = sx, y = sy, z = sz }
    end

    -- grab heart UI objects
    S.heart1 = findEntity("heart_1")
    S.heart2 = findEntity("heart_2")
    S.heart3 = findEntity("heart_3")

    -- SFX entities
    if not S.sfxHideIn  then S.sfxHideIn  = findEntity("sfx_hide_in") end
    if not S.sfxHideOut then S.sfxHideOut = findEntity("sfx_hide_out") end
    if not S.sfxRespawn then S.sfxRespawn = findEntity("sfx_respawn") end
    if not S.sfxIdle    then S.sfxIdle    = findEntity("sfx_idle") end
    if not S.sfxJump    then S.sfxJump    = findEntity("sfx_jump") end
    if not S.sfxDrop    then S.sfxDrop    = findEntity("sfx_drop_collectible") end

end

local function playSfx(e)
    if e and audioPlay then
        audioPlay(e)
    end
end

function S.update(dt)
    -- cooldown
    if S.respawnCooldown > 0 then
        S.respawnCooldown = S.respawnCooldown - dt
        if S.respawnCooldown < 0 then
            S.respawnCooldown = 0
        end
    end

    -- teleport here, outside the contact callback
    if S.pendingRespawn then
        local pr = S.pendingRespawn
        setPosition(pr.entity, pr.x, pr.y, pr.z)
        log("[PlayerState] Respawned player at:", pr.x, pr.y, pr.z)
        S.pendingRespawn = nil
    end

    -- no player, nth to do
    if not S.player then
        return
    end

    ----------------------------------------------------------------
    -- tap / double-tap handling for generic input (click/touch)
    ----------------------------------------------------------------
    if I.tapCount > 0 then
        I.tapTimer = I.tapTimer + dt

        if I.tapTimer > I.doubleTapThreshold then
            -- too slow, reset
            I.tapCount = 0
            I.tapTimer = 0.0
            I.doubleTapped = false
        elseif I.tapCount >= 2 then
            -- got 2 taps within the threshold -> double tap!
            I.doubleTapped = true
            I.tapCount = 0
            I.tapTimer = 0.0
        else
            I.doubleTapped = false
        end
    else
        I.doubleTapped = false
    end

    local tapped = false

    local px, py, pz = getPosition(S.player)

    -- keep all player SFX entities on the player
    local function syncSfx(e)
        if e then
            setPosition(e, px, py, pz)
        end
    end

    syncSfx(S.sfxHideIn)
    syncSfx(S.sfxHideOut)
    syncSfx(S.sfxRespawn)
    syncSfx(S.sfxIdle)
    syncSfx(S.sfxJump)
    syncSfx(S.sfxDrop)


    -------------------------------------------------
    -- hiding logic -> press H
    -------------------------------------------------
    if hidePressed or tapped then
        hidePressed = false -- consume key press

        if S.hidden then
            -- unhide
            S.hidden = false
            S.hiddenIn = nil

            -- restore player original scale 
            if S.playerBaseScale then
                setScale(S.player,
                    S.playerBaseScale.x,
                    S.playerBaseScale.y,
                    S.playerBaseScale.z
                )
            end

            -- restore scale of letter
            if S.carriedLetter and S.letterBaseScale then
                setScale(S.carriedLetter,
                    S.letterBaseScale.x,
                    S.letterBaseScale.y,
                    S.letterBaseScale.z
                )
            end

            -- hard reset tap state so no double tap queues a jump on unhide
            if I then
                I.doubleTapped = false
                I.tapCount = 0
                I.tapTimer = 0.0
            end

            log("[PlayerState] Player left hiding spot")

            playSfx(S.sfxHideOut)

        else
            -- try to hide: find nearest hiding_spot within radius
            local spots = getEntitiesByTag("hiding_spot")
            if spots and #spots > 0 then
                local bestSpot = nil
                local bestDistSq = S.hideRadius * S.hideRadius

                -- find nearest hiding spot
                for _, spot in ipairs(spots) do
                    local bx, by, bz = getPosition(spot)
                    local dx = px - bx
                    local dy = py - by
                    local dz = pz - bz
                    local distSq = dx*dx + dy*dy + dz*dz

                    if distSq <= bestDistSq then
                        bestDistSq = distSq
                        bestSpot = spot
                    end
                end

                if bestSpot then
                    -- snap player into box
                    local bx, by, bz = getPosition(bestSpot)
                    setPosition(S.player, bx, by, bz)

                    -- cache base scale 
                    if not S.playerBaseScale then
                        local sx, sy, sz = getScale(S.player)
                        S.playerBaseScale = { x = sx, y = sy, z = sz }
                    end

                    -- apply smaller scale while hiding
                    local factor = S.hideScaleFactor
                    local bs = S.playerBaseScale
                    setScale(S.player,
                        bs.x * factor,
                        bs.y * factor,
                        bs.z * factor
                    )

                    if S.carriedLetter then
                        if not S.letterBaseScale then
                            local lx, ly, lz = getScale(S.carriedLetter)
                            S.letterBaseScale = { x = lx, y = ly, z = lz }
                        end

                        local lbs = S.letterBaseScale
                        setScale(S.carriedLetter,
                            lbs.x * factor,
                            lbs.y * factor,
                            lbs.z * factor
                        )
                    end

                    -- reset tap state to avoid jump immediately after hiding
                    if I then
                        I.doubleTapped = false
                        I.tapCount = 0
                        I.tapTimer = 0.0
                    end

                    S.hidden = true
                    S.hiddenIn = bestSpot
                    log("[PlayerState] Player is hiding in a box")
                    playSfx(S.sfxHideIn)
                end
            end
        end
    end

    -------------------------------------------------
    -- letter collect/deliver logic -> press C
    -------------------------------------------------
    -- if already carrying a letter, keep it on player
    if S.carriedLetter then
        local off = S.carriedOffset

        -- if hidden, scale down the carried letter offset accordingly
        if S.hidden then
            local factor = S.hideScaleFactor 
            off = {
                x = off.x * factor,
                y = off.y * factor,
                z = off.z * factor
            }
        end

        setPosition(
            S.carriedLetter,
            px + off.x,
            py + off.y,
            pz + off.z
        )
        
        -- check for nearby collection point(s) 
        local collectionPoints = getEntitiesByTag("letter_collection")
        if collectionPoints and #collectionPoints > 0 then
            local bestPoint = nil
            local bestDistSq = S.deliveryRadius * S.deliveryRadius

            for _, cp in ipairs(collectionPoints) do
                local cx, cy, cz = getPosition(cp)
                local dx = px - cx
                local dy = py - cy
                local dz = pz - cz
                local distSq = dx*dx + dy*dy + dz*dz

                if distSq <= bestDistSq then
                    bestDistSq = distSq
                    bestPoint = cp
                end
            end

            -- inside delivery radius + C pressed = deliver
            if bestPoint and (collectPressed or tapped) then
                collectPressed = false

                -- snap the letter onto the collection point first (??)
                -- local cx, cy, cz = getPosition(bestPoint)
                -- setPosition(S.carriedLetter, cx, cy, cz)

                -- remove letter entity from the world
                if deleteEntity then
                    deleteEntity(S.carriedLetter)
                end

                if removeTag then
                    removeTag(S.carriedLetter, "letter_carried")
                end

                S.carriedLetter = nil
                S.lettersDelivered = (S.lettersDelivered or 0) + 1

                log(string.format(
                    "[PlayerState] Delivered letter %d / %d at collection point",
                    S.lettersDelivered,
                    S.lettersToWin or 3
                ))

                -- WIN CHECK
                if S.lettersDelivered >= (S.lettersToWin or 3) then
                    log("[PlayerState] All letters delivered! YOU WIN")

                    -- @TODO: hook win behaviour here
                    -- changeScene("WinScene")

                end
            end
        end
        -- still carrying or just delivered – dott try to pick up new letters this frame
        return

    end

    -- if hidden, skip pickup/delivery logic
    if S.hidden then
        return
    end

    -- not carrying, look for the nearest letter_collectible
    local letters = getEntitiesByTag("letter_collectible")
    if not letters or #letters == 0 then
        return
    end

    local bestLetter = nil
    local bestDistSq = S.pickupRadius * S.pickupRadius

    for _, e in ipairs(letters) do
        local lx, ly, lz = getPosition(e)
        local dx = lx - px
        local dy = ly - py
        local dz = lz - pz
        local distSq = dx*dx + dy*dy + dz*dz

        if distSq <= bestDistSq then
            bestDistSq = distSq
            bestLetter = e
        end
    end

    if bestLetter then
        if collectPressed or tapped then
            S.carriedLetter = bestLetter
            collectPressed = false -- avoid multiple logs

            removeTag(bestLetter, "letter_collectible")
            if addTag then 
                addTag(bestLetter, "letter_carried") 
            end

            if audioPlay then
                audioPlay(bestLetter)
            end

            log("[PlayerState] Collected letter on back")
        end
    end
end

function S.isHidden()
    if S.hidden then
        if I then
            I.doubleTapped = false   -- do not allow jump while hiding
            I.tapCount = 0
            I.tapTimer = 0.0
        end
        return true
    end
    return false
end

function S.canBeCaught()
    -- cannot be caught while hiding
    if S.hidden then
        return false
    end

    return S.respawnCooldown <= 0
end

function S.setCheckpoint(player, checkpointEntity)
    local x, y, z = getPosition(player)
    S.checkpointPos = { x = x, y = y, z = z }
    
    -- update which checkpoint is active
    if S.checkpointEntity and S.checkpointEntity ~= checkpointEntity then
        if removeTag then
            removeTag(S.checkpointEntity, "active_checkpoint")
        end
    end

    S.checkpointEntity = checkpointEntity

    if addTag then
        addTag(checkpointEntity, "active_checkpoint")
    end

    --log("[PlayerState] Checkpoint set at:", x, y, z)
end

-- local NORMAL_HEART_PATH = "game/textures/heart normal.png"
-- local GREY_HEART_PATH   = "game/textures/heart grey.png"

local function updateHeartsUI()
    -- if S.heart1 then setUITexture(S.heart1, "game/textures/heart normal.png") end
    -- if S.heart2 then setUITexture(S.heart2, "game/textures/heart normal.png") end
    -- if S.heart3 then setUITexture(S.heart3, "game/textures/heart normal.png") end

    -- hide hearts if > lives
    if S.lives <= 2 and S.heart3 then
        setUITexture(S.heart3, "game/textures/heart grey.png")
    end
    if S.lives <= 1 and S.heart2 then
        setUITexture(S.heart2, "game/textures/heart grey.png")
    end
    if S.lives <= 0 and S.heart1 then
        setUITexture(S.heart1, "game/textures/heart grey.png")
    end
end

function S.onCaught(player)
    -- guard, dont recatch during cooldown
    if not S.canBeCaught() then
        return
    end

    if not S.startPos then
        S.init(player)
    end

    local px, py, pz = getPosition(player)
    local deathPos = { x = px, y = py, z = pz }

    S.lives = S.lives - 1
    if S.lives < 0 then S.lives = 0 end
    updateHeartsUI()
    log("[PlayerState] Player caught! Lives left:", S.lives)
    playSfx(S.sfxRespawn)

    -- drop carried letter
    if S.carriedLetter then
        setPosition(S.carriedLetter, deathPos.x, deathPos.y, deathPos.z)
        if removeTag then removeTag(S.carriedLetter, "letter_carried") end
        if addTag then addTag(S.carriedLetter, "letter_collectible") end
        log("[PlayerState] Dropped carried letter at death position")
        playSfx(S.sfxDrop)
        S.carriedLetter = nil
    end

    -- respawn away from the trigger a bit
    local respawn = S.checkpointPos or S.startPos
    if respawn then
        -- just store, dont teleport yet cos gives mem leak
        S.pendingRespawn = {
            entity = player,
            x = respawn.x ,
            y = respawn.y,
            z = respawn.z,
        }
    end

    -- 1 second of invincibility
    S.respawnCooldown = 1.0
end

-- tick cooldown
registerUpdate(function(dt)
    S.update(dt)
end)