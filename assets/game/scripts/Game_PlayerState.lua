
-- Game_PlayerState.lua

-- Shared game state for player lives, checkpoints, and death logic
-- attach this script to sth always present, 
-- or just somehow make sure its always loaded so other scripts can call PlayerState.onCaught(player)

log("[PlayerState] Script loaded on entityId=", tostring(entityId))

-- local G = _G_root
-- print("[Game_PlayerState] load time _G.CurrentLevelName=", _G.CurrentLevelName)


-- global so other scripts can use
_G.PlayerState = _G.PlayerState or {
    lives = 3,
    startPos = nil,
    checkpointPos = nil,
    checkpointEntity = nil,
    respawnCooldown = 0.0, -- seconds of invincibility
    pendingRespawn = nil,

    player = nil, -- cache the player entity
    carriedLetter = nil, -- entity id of letter currently on back
    pickupRadius = 0.5,  -- how close player must be to pick up letter
    deliveryRadius  = 0.5, --  to drop off at collection point
    lettersDelivered = 0,  -- letters delivered so far
    lettersToWin = 3,  -- how many letters needed to win
    carriedOffset = {   -- offset of letter on the player's back
        x = 0.0,
        y = 0,
        z = 0
    },

    hidden = false, -- is player hiding
    hiddenIn = nil, -- which spot
    hideRadius = 0.5,  -- how close to hide
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

    -- end-game state
    gameEnded   = false,
    gameWon     = false,
    uiEndScreen = nil,
    spawnGraceTime = 0.0, --5.0

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

-- paths for end screens
local GAMEOVER_TEX = "game/textures/gameover.png"
local WIN_TEX = "game/textures/win screen.png"

-- !TODO: Fix current scene path being set wrongly in UIActions.lua
local CURRENT_SCENE_PATH = _G.CurrentLevelName
-- local CURRENT_SCENE_PATH = "Level1.scn"

log("[PlayerState] Current Scene Path:", _G.CurrentLevelName)
local restartPressed = false

-- Heart UI textures
local normalHeartTexture = "game/textures/heart normal.png"
local greyHeartTexture   = "game/textures/heart grey.png"

-- guard so keys arent registered twice if script reloads
if not S._keysRegistered then
    -- Debugging
    -- registerKeyUp("C", function() requestEndOverlay("win") end)

    registerKeyDown("C", function() collectPressed = true end)
    registerKeyUp("C", function() collectPressed = false end)
    registerKeyDown("H", function() 
        --log("[PlayerState] H keydown fired")
        hidePressed = true 
    end)
    registerKeyUp("H",   function() hidePressed = false end)
    registerKeyDown("R", function() restartPressed = true end)

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

-- Ensures that currentScene is always updated
local function currentScene()
    return _G.CurrentLevelName or "Level1.scn"
end


local function updateHeartsUI()
    -- Store the player's lives
    local hearts = {S.heart1, S.heart2, S.heart3}

    -- Update each heart
    for i = 1,3 do
        local heart = hearts[i]
        if heart then
            -- Set to normal heart if true, grey if false
            if i <= S.lives then
                setUITexture(heart, normalHeartTexture)
            else
                setUITexture(heart, greyHeartTexture)
            end
        end
    end
end

local function bindHeartsFromRegistry()
    local hearts = _G.UI and _G.UI.hearts
    if not hearts or #hearts < 3 then
        return false
    end

    table.sort(hearts, function(a, b) return a.x < b.x end)

    S.heart3 = (hearts[1] and hearts[1].id) or nil
    S.heart2 = (hearts[2] and hearts[2].id) or nil
    S.heart1 = (hearts[3] and hearts[3].id) or nil

    return (S.heart1 and S.heart2 and S.heart3) ~= nil
end


-- called once when we first find the player
function S.init(player)
    --log("[PlayerState] S.init called with player=" .. tostring(player))
    --log("[PlayerState] Previous S.player=" .. tostring(S.player))

    -- Clear the stored UI entity ids
    -- _G.UI = _G.UI or {}
    -- _G.UI.hearts = {}
    _G.UI = _G.UI or {}
    _G.UI.hearts = _G.UI.hearts or {}
    log("[PlayerState] UI hearts count at init:", _G.UI and _G.UI.hearts and #_G.UI.hearts)



    local oldPlayer = S.player
    S.player = player
    
    -- reset state if player changed, game ended, or first load
    local shouldReset = (oldPlayer ~= player) or S.gameEnded or S.gameWon
    
    if shouldReset then
        log("[PlayerState] Resetting game state")
        S.lives = 3
        S.lettersDelivered = 0
        S.carriedLetter = nil
        S.hidden = false
        S.hiddenIn = nil
        S.respawnCooldown = 0.0
        S.pendingRespawn = nil
        S.gameEnded = false
        S.gameWon = false
        S.spawnGraceTime = 5.0
        S.playerBaseScale = nil -- force re-cache
        S.letterBaseScale = nil
        S.startPos = nil -- force re-cache
        S.checkpointPos = nil

        -- Clear input state
        restartPressed = false
        collectPressed = false
        hidePressed = false
        if I then
            I.tapCount = 0
            I.tapTimer = 0.0
            I.doubleTapped = false
        end
    end

     -- cache start position
    if not S.startPos then
        local x, y, z = getPosition(player)
        S.startPos = { x = x, y = y, z = z }
        S.checkpointPos = { x = x, y = y, z = z }
    end

    -- cache player scale
    if not S.playerBaseScale then
        local sx, sy, sz = getScale(player)
        S.playerBaseScale = { x = sx, y = sy, z = sz }
    end

    -- alw refresh entity ref (might change after reload)
    S.sfxHideIn  = findEntity("sfx_hide_in")
    S.sfxHideOut = findEntity("sfx_hide_out")
    S.sfxRespawn = findEntity("sfx_respawn")
    S.sfxIdle    = findEntity("sfx_idle")
    S.sfxJump    = findEntity("sfx_jump")
    S.sfxDrop    = findEntity("sfx_drop_collectible")
    S.uiEndScreen = findEntity("end_screen")
    
    if S.uiEndScreen and setUITexture then
        setUITexture(S.uiEndScreen, "")
    end

    -- queue heart binding
    S.pendingHeartBind = true
    bindHeartsFromRegistry()
    updateHeartsUI()
end

local function playSfx(e)
    if e and audioPlay then
        audioPlay(e)
    end
end

-- result = either win or lose
local function requestEndOverlay(result) 
    if _G_root and _G_root.UI_OnAction then
        _G_root.UI_OnAction("game_End", nil, result)
    else
        -- set a shared global that UIActions can poll
        _G_root.GameEndState = result
    end
end

local function triggerGameOver()
    if S.gameEnded then 
        return 
    end
    S.gameEnded = true
    S.gameWon   = false
    requestEndOverlay("lose")
end

local function triggerGameWin()
    if S.gameEnded then return end
    S.gameEnded = true
    S.gameWon   = true
    requestEndOverlay("win")
end

function S.isGameEnded()
    return S.gameEnded
end

-- called by "hide" ui button
function S.onHideButton()
    hidePressed = true
end

-- called by "collect" ui button
function S.onCollectButton()
    collectPressed = true
end

-- called by ui button, decides whether this press should hide/unhide or collect/deliver
function S.onActionButton()
    if not S.player then return end
    local px, py, pz = getPosition(S.player)

    ----------------------------------------------------------------
    -- 1) If already hidden, always treat as "unhide"
    ----------------------------------------------------------------
    if S.hidden then
        hidePressed = true
        return
    end

    ----------------------------------------------------------------
    -- 2) Check if we're close enough to a hiding_spot
    ----------------------------------------------------------------
    local nearHideSpot = false
    local spots = getEntitiesByTag("hiding_spot")
    if spots and #spots > 0 then
        local bestDistSq = S.hideRadius * S.hideRadius

        for _, spot in ipairs(spots) do
            local bx, by, bz = getPosition(spot)
            local dx = px - bx
            local dy = py - by
            local dz = pz - bz
            local distSq = dx*dx + dy*dy + dz*dz

            if distSq <= bestDistSq then
                nearHideSpot = true
                break
            end
        end
    end

    if nearHideSpot then
        ----------------------------------------------------------------
        -- 3) If near a hiding spot, treat this press as "hide"
        ----------------------------------------------------------------
        hidePressed = true
    else
        ----------------------------------------------------------------
        -- 4) Otherwise, treat this press as "collect / deliver letter"
        ----------------------------------------------------------------
        collectPressed = true
    end
end

local function getPlayer()
    local p = findEntity("Player")
    return p
end

-------------------------------------------------
-- Helper: Reset input state
-------------------------------------------------
local function resetInputState()
    if I then
        I.tapCount = 0
        I.tapTimer = 0.0
        I.doubleTapped = false
    end
end


-------------------------------------------------
-- Helper: Update tap state
-------------------------------------------------
local function updateTapState(dt)
    if I.tapCount > 0 then
        I.tapTimer = I.tapTimer + dt

        if I.tapTimer > I.doubleTapThreshold then
            resetInputState()
        elseif I.tapCount >= 2 then
            I.doubleTapped = true
            I.tapCount = 0
            I.tapTimer = 0.0
        else
            I.doubleTapped = false
        end
    else
        I.doubleTapped = false
    end
end


-------------------------------------------------
-- Helper: Sync SFX positions to player
-------------------------------------------------
local function syncAllSfx(px, py, pz)
    local sfxEntities = {
        S.sfxHideIn, S.sfxHideOut, S.sfxRespawn,
        S.sfxIdle, S.sfxJump, S.sfxDrop
    }
    for _, e in ipairs(sfxEntities) do
        if e then setPosition(e, px, py, pz) end
    end
end

-------------------------------------------------
-- Helper: Find nearest entity with tag within radius
-------------------------------------------------
local function findNearestByTag(tag, px, py, pz, radius)
    local entities = getEntitiesByTag(tag)
    if not entities or #entities == 0 then return nil end

    local bestEntity = nil
    local bestDistSq = radius * radius

    for _, e in ipairs(entities) do
        local ex, ey, ez = getPosition(e)
        local dx, dy, dz = px - ex, py - ey, pz - ez
        local distSq = dx*dx + dy*dy + dz*dz

        if distSq <= bestDistSq then
            bestDistSq = distSq
            bestEntity = e
        end
    end

    return bestEntity
end


-------------------------------------------------
-- Helper: Apply hide scale factor
-------------------------------------------------
local function applyHideScale(entity, baseScale)
    local f = S.hideScaleFactor
    setScale(entity, baseScale.x * f, baseScale.y * f, baseScale.z * f)
end

-------------------------------------------------
-- Helper: Deliver carried letter
-------------------------------------------------
local function deliverLetter()
    if deleteEntity then
        deleteEntity(S.carriedLetter)
    end
    if removeTag then
        removeTag(S.carriedLetter, "letter_carried")
    end

    S.carriedLetter = nil
    S.lettersDelivered = (S.lettersDelivered or 0) + 1

    log(string.format("[PlayerState] Delivered letter %d / %d", S.lettersDelivered, S.lettersToWin or 3))

    if S.lettersDelivered >= (S.lettersToWin or 3) then
        log("[PlayerState] All letters delivered! YOU WIN")
        triggerGameWin()
    end
end


-------------------------------------------------
-- Helper: Pick up a letter
-------------------------------------------------
local function pickupLetter(letter)
    S.carriedLetter = letter

    if removeTag then removeTag(letter, "letter_collectible") end
    if addTag then addTag(letter, "letter_carried") end
    if audioPlay then audioPlay(letter) end

    log("[PlayerState] Collected letter on back")
end


-------------------------------------------------
-- Helper: Handle hide/unhide toggle
-------------------------------------------------
local function handleHideToggle(px, py, pz)
    if S.hidden then
        -- Unhide
        S.hidden = false
        S.hiddenIn = nil

        -- Restore player scale
        if S.playerBaseScale then
            setScale(S.player, S.playerBaseScale.x, S.playerBaseScale.y, S.playerBaseScale.z)
        end

        -- Restore letter scale
        if S.carriedLetter and S.letterBaseScale then
            setScale(S.carriedLetter, S.letterBaseScale.x, S.letterBaseScale.y, S.letterBaseScale.z)
        end

        resetInputState()
        log("[PlayerState] Player left hiding spot")
        playSfx(S.sfxHideOut)
    else
        -- Try to hide
        local bestSpot = findNearestByTag("hiding_spot", px, py, pz, S.hideRadius)

        if bestSpot then
            local bx, by, bz = getPosition(bestSpot)
            setPosition(S.player, bx, by, bz)

            -- Cache and apply hide scale
            if not S.playerBaseScale then
                local sx, sy, sz = getScale(S.player)
                S.playerBaseScale = { x = sx, y = sy, z = sz }
            end

            applyHideScale(S.player, S.playerBaseScale)

            -- Also scale carried letter
            if S.carriedLetter then
                if not S.letterBaseScale then
                    local lx, ly, lz = getScale(S.carriedLetter)
                    S.letterBaseScale = { x = lx, y = ly, z = lz }
                end
                applyHideScale(S.carriedLetter, S.letterBaseScale)
            end

            resetInputState()
            S.hidden = true
            S.hiddenIn = bestSpot
            log("[PlayerState] Player is hiding in a box")
            playSfx(S.sfxHideIn)
        end
    end
end


-------------------------------------------------
-- Helper: Handle letter carry/deliver/pickup
-------------------------------------------------
local function handleLetterLogic(px, py, pz)
    -- If carrying a letter
    if S.carriedLetter then
        -- Keep letter on player's back
        local off = S.carriedOffset
        if S.hidden then
            local f = S.hideScaleFactor
            off = { x = off.x * f, y = off.y * f, z = off.z * f }
        end
        setPosition(S.carriedLetter, px + off.x, py + off.y, pz + off.z)

        -- Check for delivery
        if collectPressed then
            local deliveryPoint = findNearestByTag("letter_collection", px, py, pz, S.deliveryRadius)
            if deliveryPoint then
                collectPressed = false
                deliverLetter()
            end
        end
        return  -- Don't pick up new letters while carrying
    end

    -- If hidden, skip pickup
    if S.hidden then return end

    -- Try to pick up letter
    if collectPressed then
        local letter = findNearestByTag("letter_collectible", px, py, pz, S.pickupRadius)
        if letter then
            collectPressed = false
            pickupLetter(letter)
        end
    end
end



function S.update(dt)
    CURRENT_SCENE_PATH = currentScene()

    if S.pendingHeartBind then
        if bindHeartsFromRegistry() then
            S.pendingHeartBind = false
            updateHeartsUI()
            log("[Hearts bind] success", tostring(S.heart1), tostring(S.heart2), tostring(S.heart3))
        end
    end

    if S.respawnCooldown > 0 then
        S.respawnCooldown = S.respawnCooldown - dt
        if S.respawnCooldown < 0 then
            S.respawnCooldown = 0
        end
    end

    if S.spawnGraceTime and S.spawnGraceTime > 0 then
        S.spawnGraceTime = S.spawnGraceTime - dt
        if S.spawnGraceTime < 0 then
            S.spawnGraceTime = 0
        end
    end

    -- teleport here, outside the contact callback
    if S.pendingRespawn then
        local pr = S.pendingRespawn
        setPosition(pr.entity, pr.x, pr.y, pr.z)
        log("[PlayerState] Respawned player at:", pr.x, pr.y, pr.z)
        S.pendingRespawn = nil
    end

    -- periodically refresh player reference
    local p = getPlayer()
    if not p then 
        log("[PlayerState] WARNING: Player entity not found!")
        return 
    end
    -- always update cached reference, in case entity id changed
    if S.player ~= p then
        log("[PlayerState] Player entity changed from", tostring(S.player), "to", tostring(p))
        S.player = p
    end

    -- no player, nth to do
    if not S.player then
        return
    end

    -------------------------------------------------
    -- 5. Game ended - wait for restart only
    -------------------------------------------------
    if S.gameEnded then
        if restartPressed and changeScene then
            restartPressed = false
            resetInputState()

            _G.UI = _G.UI or {}
            _G.UI.hearts = {}

            changeScene(CURRENT_SCENE_PATH)
        end
        return
    end

    -------------------------------------------------
    -- 6. Tap / double-tap handling
    -------------------------------------------------
    updateTapState(dt)

    -------------------------------------------------
    -- 7. Get player position & sync SFX
    -------------------------------------------------
    local px, py, pz = getPosition(p)
    syncAllSfx(px, py, pz)

    -------------------------------------------------
    -- 8. Hide/Unhide logic (H key)
    -------------------------------------------------
    if hidePressed then
        hidePressed = false
        handleHideToggle(px, py, pz)
    end

    -------------------------------------------------
    -- 9. Letter carry/deliver/pickup logic (C key)
    -------------------------------------------------
    handleLetterLogic(px, py, pz)
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

    if S.gameEnded then
        return false
    end

    if S.spawnGraceTime and S.spawnGraceTime > 0 then
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

function S.onCaught(player)
    -- guard, dont recatch during cooldown
    log("[PlayerState] onCaught called. canBeCaught=", tostring(S.canBeCaught and S.canBeCaught()))
    log("[PlayerState] hearts:", tostring(S.heart1), tostring(S.heart2), tostring(S.heart3))
    log("[PlayerState] lives BEFORE:", tostring(S.lives))
    if not S.canBeCaught() then
        log("[PlayerState] onCaught blocked by canBeCaught()")
        return
    end

    if not S.startPos then
        S.init(player)
    end

    local px, py, pz = getPosition(player)
    local deathPos = { x = px, y = py, z = pz }

    S.lives = S.lives - 1

    if S.lives < 0 then 
        S.lives = 0 
    end

    updateHeartsUI()
    log("[PlayerState] Player caught! Lives left:", S.lives)

    if S.lives <= 0 then 
        -- log("[PlayerState] Triggering game over")
        triggerGameOver() 
        -- log("[PlayerState] After game over")
        return 
    end
    
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
    -- print("[Game_PlayerState] runtime _G.CurrentLevelName=", _G.CurrentLevelName)
end)



-- -- called once when we first find the player 
-- function S.init(player)
--     --log("[PlayerState] S.init called with player=" .. tostring(player))
--     --log("[PlayerState] Previous S.player=" .. tostring(S.player))

--     -- Clear the stored UI entity ids
--     _G.UI = _G.UI or {}
--     _G.UI.hearts = {}

--     -- Check if current player is same as previous player (Respawn)
--     -- or non-existent (Fresh load)
--     -- local playerChanged = (S.player ~= player)

--     -- S.player = player

--     local oldPlayer = S.player
--     local playerChanged = (oldPlayer ~= nil and oldPlayer ~= player)

--     S.player = player

--     -- If we are coming into a freshly loaded scene after game over/win,
--     -- reset the core game state
--     if playerChanged or S.gameEnded or S.gameWon then
--         S.lives = 3
--         S.lettersDelivered = 0
--         S.carriedLetter = nil
--         S.hidden = false
--         S.hiddenIn = nil
--         S.respawnCooldown = 0.0
--         S.pendingRespawn = nil
--         S.gameEnded = false
--         S.gameWon = false
--         S.spawnGraceTime = 5.0

--          -- IMPORTANT: clear restart input so it doesn't instantly re-trigger
--         restartPressed = false
--         if I then
--             I.tapCount = 0
--             I.tapTimer = 0.0
--             I.doubleTapped = false
--         end

--         -- also clear any local action flags
--         collectPressed = false
--         hidePressed = false
        
--     end

--     if not S.player then
--         S.player = player
--         -- log("[PlayerState] S.player set to", tostring(player))
--     end

--     if not S.startPos then
--         local x, y, z = getPosition(player)
--         S.startPos = { x = x, y = y, z = z }
--         S.checkpointPos = { x = x, y = y, z = z }
--         -- log("[PlayerState] Init at start position:", x, y, z)
--     end

--     -- cache the original scale 
--     if not S.playerBaseScale then
--         local sx, sy, sz = getScale(player)
--         S.playerBaseScale = { x = sx, y = sy, z = sz }
--     end

--     -- grab heart UI objects
--     -- bindHearts()
--     -- updateHeartsUI()

--     -- grab heart UI objects
--     S.pendingHeartBind = true
--     bindHeartsFromRegistry()
--     updateHeartsUI()

--     -- log("[Hearts bind] ", tostring(S.heart1), tostring(S.heart2), tostring(S.heart3))

--     -- SFX entities
--     if not S.sfxHideIn  then S.sfxHideIn  = findEntity("sfx_hide_in") end
--     if not S.sfxHideOut then S.sfxHideOut = findEntity("sfx_hide_out") end
--     if not S.sfxRespawn then S.sfxRespawn = findEntity("sfx_respawn") end
--     if not S.sfxIdle    then S.sfxIdle    = findEntity("sfx_idle") end
--     if not S.sfxJump    then S.sfxJump    = findEntity("sfx_jump") end
--     if not S.sfxDrop    then S.sfxDrop    = findEntity("sfx_drop_collectible") end

--     -- end screen UI
--     if not S.uiEndScreen then
--         S.uiEndScreen = findEntity("end_screen")
--         -- ensure it starts blank
--         if S.uiEndScreen and setUITexture then
--             setUITexture(S.uiEndScreen, "")
--         end
--     end
-- end




-- function S.update(dt)
--     if S.pendingHeartBind then
--         if bindHeartsFromRegistry() then
--             S.pendingHeartBind = false
--             updateHeartsUI()
--             log("[Hearts bind] success", tostring(S.heart1), tostring(S.heart2), tostring(S.heart3))
--         end
--     end

--     if S.respawnCooldown > 0 then
--         S.respawnCooldown = S.respawnCooldown - dt
--         if S.respawnCooldown < 0 then
--             S.respawnCooldown = 0
--         end
--     end

--     if S.spawnGraceTime and S.spawnGraceTime > 0 then
--         S.spawnGraceTime = S.spawnGraceTime - dt
--         if S.spawnGraceTime < 0 then
--             S.spawnGraceTime = 0
--         end
--     end

--     -- teleport here, outside the contact callback
--     if S.pendingRespawn then
--         local pr = S.pendingRespawn
--         setPosition(pr.entity, pr.x, pr.y, pr.z)
--         log("[PlayerState] Respawned player at:", pr.x, pr.y, pr.z)
--         S.pendingRespawn = nil
--     end

--     -- periodically refresh player reference
--     local p = getPlayer()
--     if not p then 
--         log("[PlayerState] WARNING: Player entity not found!")
--         return 
--     end
--     -- always update cached reference, in case entity id changed
--     if S.player ~= p then
--         log("[PlayerState] Player entity changed from", tostring(S.player), "to", tostring(p))
--         S.player = p
--     end

--     -- no player, nth to do
--     if not S.player then
--         return
--     end

--     -- if the game has ended, wait for restart input only
--     if S.gameEnded then
--         -- simple restart triggers:
--         --   - R key on PC
--         --   - any tap/click on mobile (using the same tap system)
--         local wantRestart = restartPressed

--         if wantRestart and changeScene then
--             -- reset tap/input state so it doesn't re-trigger
--             restartPressed = false
--             I.tapCount = 0
--             I.tapTimer = 0.0
--             I.doubleTapped = false

--             changeScene(CURRENT_SCENE_PATH)
--         end

--         return -- don't run normal gameplay while on end screen
--     end


--     ----------------------------------------------------------------
--     -- tap / double-tap handling for generic input (click/touch)
--     ----------------------------------------------------------------
--     if I.tapCount > 0 then
--         I.tapTimer = I.tapTimer + dt

--         if I.tapTimer > I.doubleTapThreshold then
--             -- too slow, reset
--             I.tapCount = 0
--             I.tapTimer = 0.0
--             I.doubleTapped = false
--         elseif I.tapCount >= 2 then
--             -- got 2 taps within the threshold -> double tap!
--             I.doubleTapped = true
--             I.tapCount = 0
--             I.tapTimer = 0.0
--         else
--             I.doubleTapped = false
--         end
--     else
--         I.doubleTapped = false
--     end

--     local tapped = false

--     local px, py, pz = getPosition(p)

--     -- keep all player SFX entities on the player
--     local function syncSfx(e)
--         if e then
--             setPosition(e, px, py, pz)
--         end
--     end

--     syncSfx(S.sfxHideIn)
--     syncSfx(S.sfxHideOut)
--     syncSfx(S.sfxRespawn)
--     syncSfx(S.sfxIdle)
--     syncSfx(S.sfxJump)
--     syncSfx(S.sfxDrop)


--     -------------------------------------------------
--     -- hiding logic -> press H
--     -------------------------------------------------
--     if hidePressed or tapped then
--         hidePressed = false -- consume key press
--         -- log("[PlayerState] Hide pressed, S.player=" .. tostring(S.player))
--         -- log("[PlayerState] S.hidden=" .. tostring(S.hidden))

--         if S.hidden then
--             -- unhide
--             S.hidden = false
--             S.hiddenIn = nil

--             -- restore player original scale 
--             if S.playerBaseScale then
--                 setScale(S.player,
--                     S.playerBaseScale.x,
--                     S.playerBaseScale.y,
--                     S.playerBaseScale.z
--                 )
--             end

--             -- restore scale of letter
--             if S.carriedLetter and S.letterBaseScale then
--                 setScale(S.carriedLetter,
--                     S.letterBaseScale.x,
--                     S.letterBaseScale.y,
--                     S.letterBaseScale.z
--                 )
--             end

--             -- hard reset tap state so no double tap queues a jump on unhide
--             if I then
--                 I.doubleTapped = false
--                 I.tapCount = 0
--                 I.tapTimer = 0.0
--             end

--             log("[PlayerState] Player left hiding spot")

--             playSfx(S.sfxHideOut)

--         else
--             -- try to hide: find nearest hiding_spot within radius
--             local spots = getEntitiesByTag("hiding_spot")
--             -- log("[PlayerState] Found " .. tostring(spots and #spots or 0) .. " hiding_spot entities")
--             -- if spots then
--             --     for i, spot in ipairs(spots) do
--             --         local bx, by, bz = getPosition(spot)
--             --         log("[PlayerState] Spot " .. i .. " entity=" .. tostring(spot) .. " pos=(" .. bx .. "," .. by .. "," .. bz .. ")")
--             --     end
--             -- end

--             if spots and #spots > 0 then

--                 --log("[PlayerState] hidePressed. player=", tostring(S.player), " spots=", tostring(#spots))
--                 local bestSpot = nil
--                 local bestDistSq = S.hideRadius * S.hideRadius

--                 -- find nearest hiding spot
--                 for _, spot in ipairs(spots) do
--                     local bx, by, bz = getPosition(spot)
--                     local dx = px - bx
--                     local dy = py - by
--                     local dz = pz - bz
--                     local distSq = dx*dx + dy*dy + dz*dz

--                     -- log(string.format("[HideCheck] px=%.2f py=%.2f pz=%.2f | bx=%.2f by=%.2f bz=%.2f | dist=%.3f (r=%.3f)",
--                     --                     px,py,pz, bx,by,bz, math.sqrt(distSq), S.hideRadius))

--                     if distSq <= bestDistSq then
--                         bestDistSq = distSq
--                         bestSpot = spot
--                     end
--                 end

--                 if bestSpot then
--                     -- snap player into box
--                     local bx, by, bz = getPosition(bestSpot)
--                     setPosition(S.player, bx, by, bz)

--                     -- cache base scale 
--                     if not S.playerBaseScale then
--                         local sx, sy, sz = getScale(S.player)
--                         S.playerBaseScale = { x = sx, y = sy, z = sz }
--                     end

--                     -- apply smaller scale while hiding
--                     local factor = S.hideScaleFactor
--                     local bs = S.playerBaseScale
--                     setScale(S.player,
--                         bs.x * factor,
--                         bs.y * factor,
--                         bs.z * factor
--                     )

--                     if S.carriedLetter then
--                         if not S.letterBaseScale then
--                             local lx, ly, lz = getScale(S.carriedLetter)
--                             S.letterBaseScale = { x = lx, y = ly, z = lz }
--                         end

--                         local lbs = S.letterBaseScale
--                         setScale(S.carriedLetter,
--                             lbs.x * factor,
--                             lbs.y * factor,
--                             lbs.z * factor
--                         )
--                     end

--                     -- reset tap state to avoid jump immediately after hiding
--                     if I then
--                         I.doubleTapped = false
--                         I.tapCount = 0
--                         I.tapTimer = 0.0
--                     end

--                     S.hidden = true
--                     S.hiddenIn = bestSpot
--                     log("[PlayerState] Player is hiding in a box")
--                     playSfx(S.sfxHideIn)
--                 end
--             end
--         end
--     end

--     -------------------------------------------------
--     -- letter collect/deliver logic -> press C
--     -------------------------------------------------
--     -- if already carrying a letter, keep it on player
--     if S.carriedLetter then
--         local off = S.carriedOffset

--         -- if hidden, scale down the carried letter offset accordingly
--         if S.hidden then
--             local factor = S.hideScaleFactor 
--             off = {
--                 x = off.x * factor,
--                 y = off.y * factor,
--                 z = off.z * factor
--             }
--         end

--         setPosition(
--             S.carriedLetter,
--             px + off.x,
--             py + off.y,
--             pz + off.z
--         )
        
--         -- check for nearby collection point(s) 
--         local collectionPoints = getEntitiesByTag("letter_collection")
--         if collectionPoints and #collectionPoints > 0 then
--             local bestPoint = nil
--             local bestDistSq = S.deliveryRadius * S.deliveryRadius

--             for _, cp in ipairs(collectionPoints) do
--                 local cx, cy, cz = getPosition(cp)
--                 local dx = px - cx
--                 local dy = py - cy
--                 local dz = pz - cz
--                 local distSq = dx*dx + dy*dy + dz*dz

--                 if distSq <= bestDistSq then
--                     bestDistSq = distSq
--                     bestPoint = cp
--                 end
--             end

--             -- inside delivery radius + C pressed = deliver
--             if bestPoint and (collectPressed or tapped) then
--                 collectPressed = false

--                 -- snap the letter onto the collection point first (??)
--                 -- local cx, cy, cz = getPosition(bestPoint)
--                 -- setPosition(S.carriedLetter, cx, cy, cz)

--                 -- remove letter entity from the world
--                 if deleteEntity then
--                     deleteEntity(S.carriedLetter)
--                 end

--                 if removeTag then
--                     removeTag(S.carriedLetter, "letter_carried")
--                 end

--                 S.carriedLetter = nil
--                 S.lettersDelivered = (S.lettersDelivered or 0) + 1

--                 log(string.format(
--                     "[PlayerState] Delivered letter %d / %d at collection point",
--                     S.lettersDelivered,
--                     S.lettersToWin or 3
--                 ))

--                 -- WIN CHECK
--                 if S.lettersDelivered >= (S.lettersToWin or 3) then
--                     log("[PlayerState] All letters delivered! YOU WIN")
--                     triggerGameWin()
--                 end
--             end
--         end
--         -- still carrying or just delivered – dott try to pick up new letters this frame
--         return

--     end

--     -- if hidden, skip pickup/delivery logic
--     if S.hidden then
--         return
--     end

--     -- not carrying, look for the nearest letter_collectible
--     local letters = getEntitiesByTag("letter_collectible")
--     if not letters or #letters == 0 then
--         return
--     end

--     local bestLetter = nil
--     local bestDistSq = S.pickupRadius * S.pickupRadius

--     for _, e in ipairs(letters) do
--         local lx, ly, lz = getPosition(e)
--         local dx = lx - px
--         local dy = ly - py
--         local dz = lz - pz
--         local distSq = dx*dx + dy*dy + dz*dz

--         if distSq <= bestDistSq then
--             bestDistSq = distSq
--             bestLetter = e
--         end
--     end

--     if bestLetter then
--         if collectPressed or tapped then
--             S.carriedLetter = bestLetter
--             collectPressed = false -- avoid multiple logs

--             removeTag(bestLetter, "letter_collectible")
--             if addTag then 
--                 addTag(bestLetter, "letter_carried") 
--             end

--             if audioPlay then
--                 audioPlay(bestLetter)
--             end

--             log("[PlayerState] Collected letter on back")
--         end
--     end
-- end