
-- Game_PlayerState.lua

-- Shared game state for player lives, checkpoints, and death logic
-- attach this script to sth always present, 
-- or just somehow make sure its always loaded so other scripts can call PlayerState.onCaught(player)

log("[PlayerState] Script loaded on entityId=", tostring(entityId))

-- Game Over SFX file paths
local SFX_GAMEOVER_HIT = "game/audio/bgm/gameover bgm/GameOver Hit 2.wav"
local SFX_GAMEOVER_LOOP = "game/audio/bgm/gameover bgm/GameOver Loop.wav"
local gameOverLoopChannel = -1  -- Track looping channel for stopping on restart

-- New SFX Paths
local SFX_RESPAWN = "game/audio/sfx/player/Player_Respawn_01.wav"
local SFX_HIDE_IN = "game/audio/sfx/player/hide/Box In.wav"
local SFX_HIDE_OUT = "game/audio/sfx/player/hide/Box Out.wav"

-- SFX Volumes (decibel modifers)
local VOL_GAMEOVER_HIT = 0.0
local VOL_GAMEOVER_LOOP = 0.0
local VOL_RESPAWN = -3.0
local VOL_HIDE = 0.0


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
    lettersToWin = 1,  -- how many letters needed to win
    carriedOffset = {   -- offset of letter on the player's back
        x = 0.0,
        y = 0,
        z = 0
    },

    hidden = false, -- is player hiding
    hiddenIn = nil, -- which spot
    hideRadius = 1,  -- how close to hide
    playerBaseScale = nil,   -- original scale of player
    letterBaseScale = nil,
    hideScaleFactor = 0.1,   -- how small when hiding 

    -- SFX entities (Legacy entities removed)
    -- sfxHideIn  = nil,
    -- sfxHideOut = nil,
    -- sfxRespawn = nil,
    -- sfxIdle    = nil,
    -- sfxJump    = nil,
    -- sfxDrop    = nil,

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

-- !TODO: Fix current scene path being set wrongly in UIActions.lua
local CURRENT_SCENE_PATH = _G.CurrentLevelName
local NEXT_SCENE_PATH = _G.NextLevelName

log("[PlayerState] Current Scene Path:", _G.CurrentLevelName)
log("[PlayerState] Next Scene Path:", _G.NextLevelName)
local restartPressed = false

-- Heart UI textures
local normalHeartTexture = "game/textures/heart normal.png"
local greyHeartTexture   = "game/textures/heart grey.png"

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
    
    -- Play Game Over SFX: hit sound once, then looping background
    audioPlaySFX(SFX_GAMEOVER_HIT, VOL_GAMEOVER_HIT)
    gameOverLoopChannel = audioPlaySFX(SFX_GAMEOVER_LOOP, VOL_GAMEOVER_LOOP, true)

    -- Duck Global BGM
    if _G.GlobalAudio and _G.GlobalAudio.setGameOver then
        _G.GlobalAudio.setGameOver(true)
    end
    
    requestEndOverlay("lose")
end

local function triggerGameWin()
    if S.gameEnded then return end
    S.gameEnded = true
    S.gameWon   = true
    requestEndOverlay("win")
end

-- guard so keys arent registered twice if script reloads
if not S._keysRegistered then
    -- Debugging
    -- registerKeyUp("C", function() triggerGameWin() end)

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
    return _G.CurrentLevelName 
end

local function nextScene()
    return _G.NextLevelName 
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

    if addTag and S.player then
        addTag(S.player, "Player")
    end
    
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

        -- Stop Game Over loop if it's playing
        if gameOverLoopChannel >= 0 then
            if audioStopChannel then audioStopChannel(gameOverLoopChannel) end
            gameOverLoopChannel = -1
        end

        -- Restore Global BGM volume check (failsafe)
        if _G.GlobalAudio and _G.GlobalAudio.setGameOver then
            _G.GlobalAudio.setGameOver(false)
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
    -- Legacy entity lookups removed
    -- S.sfxHideIn  = findEntity("sfx_hide_in")
    -- S.sfxHideOut = findEntity("sfx_hide_out")
    -- S.sfxRespawn = findEntity("sfx_respawn")
    -- S.sfxIdle    = findEntity("sfx_idle")
    -- S.sfxJump    = findEntity("sfx_jump")
    -- S.sfxDrop    = findEntity("sfx_drop_collectible")
    S.uiEndScreen = findEntity("end_screen")
    
    if S.uiEndScreen and setUITexture then
        setUITexture(S.uiEndScreen, "")
    end

    -- queue heart binding
    S.pendingHeartBind = true
    bindHeartsFromRegistry()
    updateHeartsUI()
end



-- result = either win or lose
-- local function requestEndOverlay(result) 
--     if _G_root and _G_root.UI_OnAction then
--         _G_root.UI_OnAction("game_End", nil, result)
--     else
--         -- set a shared global that UIActions can poll
--         _G_root.GameEndState = result
--     end
-- end

-- local function triggerGameOver()
--     if S.gameEnded then 
--         return 
--     end
--     S.gameEnded = true
--     S.gameWon   = false
--     requestEndOverlay("lose")
-- end

-- local function triggerGameWin()
--     if S.gameEnded then return end
--     S.gameEnded = true
--     S.gameWon   = true
--     requestEndOverlay("win")
-- end

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
-- Helper: Find nearest entity with tag within radius
-------------------------------------------------
local function findNearestByTag(tag, px, py, pz, radius)
    local entities = getEntitiesByTag(tag)
    if not entities or #entities == 0 then return nil end

    local bestEntity = nil
    local bestDistSq = radius * radius

    for _, e in ipairs(entities) do
        local ex, ey, ez = getPosition(e)
        if ex and ey and ez then
            local dx, dy, dz = px - ex, py - ey, pz - ez
            local distSq = dx*dx + dy*dy + dz*dz

            if distSq <= bestDistSq then
                bestDistSq = distSq
                bestEntity = e
            end
        end
    end

    return bestEntity
end

local function ensureMinimapGameplayTags()
    if not addTag then
        return
    end

    local collectibles = getEntitiesByTag("letter_collectible") or {}
    for _, e in ipairs(collectibles) do
        addTag(e, "item")
    end

    local carried = getEntitiesByTag("letter_carried") or {}
    for _, e in ipairs(carried) do
        addTag(e, "item")
    end

    local objectives = getEntitiesByTag("letter_collection") or {}
    for _, e in ipairs(objectives) do
        addTag(e, "objective")
    end
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

    disablePhysics(letter)

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
        resetInputState()
        log("[PlayerState] Player left hiding spot")
        -- NEW: Play hide out sound at the box location
        if S.hiddenIn then
            audioPlaySFXFromEntity(SFX_HIDE_OUT, S.hiddenIn, VOL_HIDE)
        end
    else
        -- Try to hide
        local bestSpot = findNearestByTag("hiding_spot", px, py, pz, S.hideRadius)

        if bestSpot then
            local bx, by, bz = getPosition(bestSpot)
            setPosition(S.player, bx, by, bz)
            setVelocity(S.player, 0.0, 0.0, 0.0)
            log("[PlayerState] SET VEL TO 0")
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
            S.hidden = true
            S.hiddenIn = bestSpot
            log("[PlayerState] Player is hiding in a box")
            -- NEW: Play hide in sound at the box location
            audioPlaySFXFromEntity(SFX_HIDE_IN, bestSpot, VOL_HIDE)
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
    NEXT_SCENE_PATH = nextScene()

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

            if S.gameWon then 
                -- Player win, go to next level
                changeScene(NEXT_SCENE_PATH)
            else
                -- Player lose, restart level
                _G.UI = _G.UI or {}
                _G.UI.hearts = {}

                changeScene(CURRENT_SCENE_PATH)
            end
        end
        return
    end

    -------------------------------------------------
    -- 6. Tap / double-tap handling
    -------------------------------------------------
    updateTapState(dt)

    -------------------------------------------------
    -- 7. Get player position
    -------------------------------------------------
    local px, py, pz = getPosition(p)

    -- keep gameplay-critical minimap tags in sync
    ensureMinimapGameplayTags()

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
        triggerGameOver() 
        return 
    end
    
    -- Play respawn SFX at player position
    audioPlaySFXFromEntity(SFX_RESPAWN, player, VOL_RESPAWN)

    -- drop carried letter
    if S.carriedLetter then
        setPosition(S.carriedLetter, deathPos.x, deathPos.y, deathPos.z)
        enablePhysics(S.carriedLetter)
        if removeTag then removeTag(S.carriedLetter, "letter_carried") end
        if addTag then addTag(S.carriedLetter, "letter_collectible") end
        log("[PlayerState] Dropped carried letter at death position")
        -- REMOVED: playSfx(S.sfxDrop)
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
