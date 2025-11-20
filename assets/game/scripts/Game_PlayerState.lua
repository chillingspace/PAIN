
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
    pickupRadius = 4.0,  -- how close player must be
    carriedOffset = {   -- offset of letter on the player's back
        x = 0.0,
        y = 1.2,
        z = -0.3
    },

    _keysRegistered = false
}

local S = _G.PlayerState
local collectPressed = false

-- guard so keys arent registered twice if script reloads
if not S._keysRegistered then
    registerKeyDown("C", function()
        collectPressed = true
    end)

    registerKeyUp("C", function()
        collectPressed = false
    end)

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

    -- collectible logic:

    -- no player, nth to do
    if not S.player then
        return
    end

    local px, py, pz = getPosition(S.player)

    -- if already carrying a letter, keep it on player
    if S.carriedLetter then
        local off = S.carriedOffset
        setPosition(
            S.carriedLetter,
            px + off.x,
            py + off.y,
            pz + off.z
        )
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
        -- pickup, C key
        if collectPressed then
            S.carriedLetter = bestLetter
            collectPressed = false -- avoid multiple logs

            removeTag(bestLetter, "letter_collectible")
            if addTag then 
                addTag(bestLetter, "letter_carried") 
            end

            log("[PlayerState] Collected letter on back")
        end
    end
end

function S.canBeCaught()
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
    log("[PlayerState] Player caught! Lives left:", S.lives)

    -- DROP CARRIED LETTER HERE 
    if S.carriedLetter then
        setPosition(S.carriedLetter, deathPos.x, deathPos.y, deathPos.z)
        if removeTag then removeTag(S.carriedLetter, "letter_carried") end
        if addTag then addTag(S.carriedLetter, "letter_collectible") end
        log("[PlayerState] Dropped carried letter at death position")
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