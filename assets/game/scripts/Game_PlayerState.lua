
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
local SFX_RESPAWN   = "game/audio/sfx/player/Player_Respawn_01.wav"
local SFX_HIDE_IN   = "game/audio/sfx/player/hide/Box In.wav"
local SFX_HIDE_OUT  = "game/audio/sfx/player/hide/Box Out.wav"
local SFX_PIPE_IN   = "game/audio/sfx/player/hide/Box In.wav"   -- swap pipe sfx later if have
local SFX_PIPE_OUT  = "game/audio/sfx/player/hide/Box Out.wav"
local SFX_COLLECT   = "game/audio/sfx/Gear Pick Up.wav"
local SFX_DELIVER   = "game/audio/sfx/Gear Put Down.wav"
local SFX_ROBOT_INTERACT = "game/audio/sfx/cutscenes/gear turning cutscene.wav"
local SFX_WIN_MUSIC = "game/audio/music/WinScreen_v1.wav"

-- Pipe group suffixes — tag pipe ends as "pipe_entrance_A" / "pipe_exit_A", etc.
local PIPE_GROUPS = { "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
                      "P","Q","R","S","T","U","V","W","X","Y","Z" }

-- SFX Volumes (decibel modifers)
local VOL_GAMEOVER_HIT = 0.0
local VOL_GAMEOVER_LOOP = 0.0
local VOL_RESPAWN = -3.0
local VOL_HIDE = 0.0
local VOL_COLLECT = 0.0
local VOL_DELIVER = 0.0
local VOL_ROBOT_INTERACT = 0.0
local VOL_WIN_MUSIC = -9.0  -- Original is very loud


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
    collectibleOriginPos = nil, -- world pos of gear entity at moment of pickup
    pickupRadius = 0.5,  -- how close player must be to pick up letter
    deliveryRadius  = 0.5, --  to drop off at collection point
    lettersDelivered = 0,  -- letters delivered so far
    lettersToWin = 1,  -- how many letters needed to win
    carriedOffset = {   -- offset of letter on the player's back
        x = 0.0,
        y = 0,
        z = 0
    },
    deliverySequenceActive = false,
    deliveryObjective = nil,
    deliverySparkElapsed = 0.0,
    deliverySparkConfig = nil,
    deliveryLightScale = 0.0,

    hidden = false, -- is player hiding
    hiddenIn = nil, -- which spot
    hideRadius = 2,  -- how close to hide
    playerBaseScale = nil,   -- original scale of player
    letterBaseScale = nil,
    hideScaleFactor = 0.1,   -- how small when hiding

    -- pipe traversal state
    inPipe          = false,
    pipeEntrancePos = nil,   -- { x, y, z } of the entrance end
    pipeExitPos     = nil,   -- { x, y, z } of the exit end
    pipeDirX        = 0.0,   -- unit vector from entrance → exit
    pipeDirY        = 0.0,
    pipeDirZ        = 0.0,
    pipeLen         = 0.0,   -- world-space length of pipe
    pipeEnterRadius = 0.7,   -- how close the player must be to enter a pipe

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

    -- interaction prompt billboard entities
    promptEntities = {},   -- list of { entity=<id>, ptype="pipe"|"hide"|"collect" }

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
local normalHeartTexture = "game/textures/heart_red.png"
local greyHeartTexture   = "game/textures/heart_grey.png"

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
    -- Register in global SFX registry for scene-change cleanup
    if gameOverLoopChannel >= 0 then
        _G.SFXChannels = _G.SFXChannels or {}
        _G.SFXChannels[gameOverLoopChannel] = true
    end

    -- Duck Global BGM
    if _G.GlobalAudio and _G.GlobalAudio.setGameOver then
        _G.GlobalAudio.setGameOver(true)
    end
    SetGamePaused(S.gameEnded) 
    requestEndOverlay("lose")
end

local function triggerGameWin()
    if S.gameEnded then return end
    S.gameEnded = true
    S.gameWon   = true

    -- Play win screen music (one-shot, no loop)
    if audioPlaySFX then audioPlaySFX(SFX_WIN_MUSIC, VOL_WIN_MUSIC) end

    SetGamePaused(S.gameWon) 
    requestEndOverlay("win")
end

-- Global cleanup: stop game-over loop SFX and restore BGM ducking.
-- Called from UIActions.lua before scene changes, and from S.init as failsafe.
function S.stopGameOverAudio()
    if gameOverLoopChannel >= 0 then
        if audioStopChannel then audioStopChannel(gameOverLoopChannel) end
        -- Unregister from global SFX registry
        if _G.SFXChannels then _G.SFXChannels[gameOverLoopChannel] = nil end
        gameOverLoopChannel = -1
    end
    if _G.GlobalAudio and _G.GlobalAudio.setGameOver then
        _G.GlobalAudio.setGameOver(false)
    end
end

-- guard so keys arent registered twice if script reloads
if not S._keysRegistered then
    -- Debugging
    registerKeyUp("P", function() triggerGameWin() end)

    registerKeyDown("C", function() collectPressed = true end)
    registerKeyUp("C", function() collectPressed = false end)

    registerKeyDown("R", function() restartPressed = true end)

    -- Dynamic dispatch for the rebindable "hide" key
    local KB_PS = _G.KeyBindings
    if KB_PS then
        -- All keys that could possibly be bound to hide
        local ALL_KEYS_PS = {
            "A","B","C","D","E","F","G","H","I","J","K","L","M",
            "N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
            "0","1","2","3","4","5","6","7","8","9",
            "SPACE","TAB","ENTER",
            "LSHIFT","RSHIFT","LCTRL","RCTRL","LALT","RALT","CAPSLOCK",
            "KEY_U","KEY_D","KEY_L","KEY_R",
            "COMMA","PERIOD","SLASH","SEMICOLON","APOSTROPHE",
            "EQUAL","MINUS","LBRACKET","RBRACKET","BACKSLASH","GRAVE",
            "DELETE","INSERT","HOME","END","PAGEUP","PAGEDOWN"
        }
        for _, kn in ipairs(ALL_KEYS_PS) do
            registerKeyDown(kn, function()
                if KB_PS.hide == kn then
                    hidePressed = true
                end
            end)
            registerKeyUp(kn, function()
                if KB_PS.hide == kn then
                    hidePressed = false
                end
            end)
        end
    else
        -- Fallback: hardcoded E key if KeyBindings not available
        registerKeyDown("E", function() hidePressed = true end)
        registerKeyUp("E", function() hidePressed = false end)
    end

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
        S.collectibleOriginPos = nil
        S.hidden = false
        S.hiddenIn = nil
        S.inPipe = false
        S.pipeEntrancePos = nil
        S.pipeExitPos = nil
        S.respawnCooldown = 0.0
        S.pendingRespawn = nil
        S.gameEnded = false
        S.gameWon = false
        S.deliverySequenceActive = false
        S.deliveryObjective = nil
        S.deliverySparkElapsed = 0.0
        S.deliverySparkConfig = nil
        S.deliveryLightScale = 0.0
        S.spawnGraceTime = 5.0
        S.playerBaseScale = nil -- force re-cache
        S.letterBaseScale = nil
        S.startPos = nil -- force re-cache
        S.checkpointPos = nil
        S.promptEntities = {}   -- force re-discovery of prompt billboard entities

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
        S.stopGameOverAudio()
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

    -- Discover all interaction prompt billboard entities and hide them
    S.promptEntities = {}
    local allPrompts = getEntitiesByTag and getEntitiesByTag("interaction_prompt") or {}
    for _, e in ipairs(allPrompts) do
        local ptype
        if hasTag(e, "prompt_pipe") then
            ptype = "pipe"
        elseif hasTag(e, "prompt_hide") then
            ptype = "hide"
        elseif hasTag(e, "prompt_collect") then
            ptype = "collect"
        end
        -- Resolve the interactable entity this prompt follows (for distance checks)
        local targetId = getUIFollowTarget and getUIFollowTarget(e) or nil
        if ptype and targetId then
            table.insert(S.promptEntities, { entity = e, ptype = ptype, target = targetId })
        end
        setUIFollowEnabled(e, false)
    end
    log("[PlayerState] Interaction prompts found:", #S.promptEntities)

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
    -- 1) Exit pipe
    ----------------------------------------------------------------
    if S.inPipe then
        hidePressed = true
        return
    end

    ----------------------------------------------------------------
    -- 2) If already hidden in a box, unhide
    ----------------------------------------------------------------
    if S.hidden then
        hidePressed = true
        return
    end

    ----------------------------------------------------------------
    -- 3) Near a pipe entrance → enter pipe
    ----------------------------------------------------------------
    if findNearestPipeEntrance(px, py, pz) then
        hidePressed = true
        return
    end

    ----------------------------------------------------------------
    -- 4) Near a hiding spot → hide
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
            if dx*dx + dy*dy + dz*dz <= bestDistSq then
                nearHideSpot = true
                break
            end
        end
    end

    if nearHideSpot then
        hidePressed = true
    else
        ----------------------------------------------------------------
        -- 5) Otherwise collect / deliver letter
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
-- Helper: Delivery sequence + completion
-------------------------------------------------
local function getDeliverySequenceConfig()
    local scenePath = currentScene() or CURRENT_SCENE_PATH or ""
    if string.find(scenePath, "Level 2.scn", 1, true) then
        return {
            phase = "success_ignite",
            panDuration = 0.65,
            holdDuration = 1.85,
            returnDuration = 0.75,
            camDistance = 3.9,
            camHeight = 2.0,
            lookHeight = 1.2,
            lightOffsetY = 1.05,
            lightColor = { r = 2.2, g = 1.55, b = 0.75 },
            idleLightScale = 0.68,
            keepLightOn = true,
        }
    elseif string.find(scenePath, "Level1.scn", 1, true) then
        return {
            phase = "unstable_reboot",
            panDuration = 0.55,
            holdDuration = 1.45,
            returnDuration = 0.65,
            camDistance = 3.7,
            camHeight = 1.85,
            lookHeight = 1.1,
            lightOffsetY = 0.95,
            lightColor = { r = 1.45, g = 0.95, b = 0.4 },
            idleLightScale = 0.0,
            keepLightOn = false,
        }
    end

    return {
        phase = "failed_flicker",
        panDuration = 0.45,
        holdDuration = 1.2,
        returnDuration = 0.55,
        camDistance = 3.4,
        camHeight = 1.65,
        lookHeight = 1.0,
        lightOffsetY = 0.9,
        lightColor = { r = 1.1, g = 0.72, b = 0.28 },
        idleLightScale = 0.0,
        keepLightOn = false,
    }
end

local function lerp(a, b, t)
    return a + (b - a) * t
end

local function normalizeVector3(x, y, z)
    local len = math.sqrt((x * x) + (y * y) + (z * z))
    if len < 0.0001 then
        return 0.0, 0.0, 1.0
    end
    return x / len, y / len, z / len
end

local function hasClearDeliveryView(camX, camY, camZ, lookX, lookY, lookZ)
    if not hasLineOfSight then
        return true
    end
    return hasLineOfSight(camX, camY, camZ, lookX, lookY, lookZ)
end

local function selectDeliveryCameraShot(deliveryPoint, px, py, pz, cfg)
    local rx, ry, rz = getPosition(deliveryPoint)
    local lookX = rx
    local lookY = ry + cfg.lookHeight
    local lookZ = rz

    local liveCam = _G.CurrentGameplayCamera
    local camDirX, camDirY, camDirZ
    local shotTag = "live_camera"

    if liveCam and liveCam.px and liveCam.py and liveCam.pz then
        camDirX = liveCam.px - px
        camDirY = liveCam.py - py
        camDirZ = liveCam.pz - pz
    else
        local fallbackX = px - rx
        local fallbackY = cfg.camHeight
        local fallbackZ = pz - rz
        camDirX, camDirY, camDirZ = fallbackX, fallbackY, fallbackZ
        shotTag = "fallback_camera"
    end

    camDirX, camDirY, camDirZ = normalizeVector3(camDirX, camDirY, camDirZ)

    local desiredX = lookX + camDirX * cfg.camDistance
    local desiredY = lookY + camDirY * cfg.camDistance
    local desiredZ = lookZ + camDirZ * cfg.camDistance

    local camX, camY, camZ = desiredX, desiredY, desiredZ
    if cameraGetPositionWithCollision then
        camX, camY, camZ = cameraGetPositionWithCollision(
            lookX, lookY, lookZ,
            desiredX, desiredY, desiredZ,
            S.player or deliveryPoint
        )
        shotTag = shotTag .. "_collision"
    end

    if not hasClearDeliveryView(camX, camY, camZ, lookX, lookY, lookZ) then
        return desiredX, desiredY, desiredZ, lookX, lookY, lookZ, shotTag .. "_blocked"
    end

    return camX, camY, camZ, lookX, lookY, lookZ, shotTag
end

local function ensureDeliveryLight()
    local deliveryObjective = S.deliveryObjective
    local sparkCfg = S.deliverySparkConfig
    if not deliveryObjective or not sparkCfg or not addLight then
        return false
    end

    if hasLight and not hasLight(deliveryObjective) then
        addLight(deliveryObjective)
    elseif not hasLight then
        addLight(deliveryObjective)
    end

    if setLightType then
        setLightType(deliveryObjective, 0)
    end
    if setShadowType then
        setShadowType(deliveryObjective, 0)
    end
    if setLightPosition then
        setLightPosition(deliveryObjective, 0.0, sparkCfg.lightOffsetY or 0.9, 0.0)
    end
    return true
end

local function setDeliveryLight(scale, colorOverride)
    local deliveryObjective = S.deliveryObjective
    local sparkCfg = S.deliverySparkConfig
    if not deliveryObjective or not sparkCfg or not setLightIntensity then
        return
    end

    if setLightPosition then
        setLightPosition(deliveryObjective, 0.0, sparkCfg.lightOffsetY or 0.9, 0.0)
    end

    local color = colorOverride or sparkCfg.lightColor or { r = 1.0, g = 1.0, b = 1.0 }
    local clampedScale = math.max(0.0, scale or 0.0)
    setLightIntensity(deliveryObjective, color.r * clampedScale, color.g * clampedScale, color.b * clampedScale)
    S.deliveryLightScale = clampedScale
end

local function updateDeliverySequenceEffects(dt)
    local cfg = S.deliverySparkConfig
    if not S.deliverySequenceActive or not cfg or not S.deliveryObjective then
        return
    end

    ensureDeliveryLight()

    local t = S.deliverySparkElapsed or 0.0
    local lightScale = 0.0

    if cfg.phase == "failed_flicker" then
        if t < 0.16 then
            lightScale = lerp(0.0, 0.55, t / 0.16)
        elseif t < 0.28 then
            lightScale = 0.2
        elseif t < 0.4 then
            lightScale = 0.0
        elseif t < 0.6 then
            lightScale = 0.42
        elseif t < 0.9 then
            lightScale = lerp(0.42, 0.0, (t - 0.6) / 0.3)
        end
    elseif cfg.phase == "unstable_reboot" then
        if t < 0.18 then
            lightScale = lerp(0.0, 0.72, t / 0.18)
        elseif t < 0.36 then
            lightScale = 0.3
        elseif t < 0.62 then
            lightScale = 0.95
        elseif t < 0.86 then
            lightScale = 0.24
        elseif t < 1.08 then
            lightScale = 0.82
        elseif t < 1.42 then
            lightScale = lerp(0.82, 0.12, (t - 1.08) / 0.34)
        elseif t < 1.8 then
            lightScale = lerp(0.12, 0.0, (t - 1.42) / 0.38)
        end
    else
        if t < 0.34 then
            lightScale = lerp(0.12, 0.75, t / 0.34)
        elseif t < 0.7 then
            lightScale = lerp(0.75, 1.3, (t - 0.34) / 0.36)
        elseif t < 1.0 then
            lightScale = 1.45
        elseif t < 1.16 then
            lightScale = 2.1
        else
            lightScale = cfg.idleLightScale or 0.82
        end
    end

    setDeliveryLight(lightScale)

end

local function completeDeliverySequence()
    local deliveredLetter = S.carriedLetter
    local deliveryObjective = S.deliveryObjective
    local deliveryCfg = S.deliverySparkConfig

    if deliveryObjective and setLightIntensity then
        if deliveryCfg and deliveryCfg.keepLightOn then
            setDeliveryLight(deliveryCfg.idleLightScale or 0.82)
        else
            setLightIntensity(deliveryObjective, 0.0, 0.0, 0.0)
        end
    end

    S.deliverySparkElapsed = 0.0
    S.deliverySparkConfig = nil
    S.deliveryLightScale = 0.0

    if deliveredLetter then
        if deleteEntity then
            deleteEntity(deliveredLetter)
        end
        if removeTag then
            removeTag(deliveredLetter, "letter_carried")
        end
    end

    SetModel(S.player, "Frog_Anim.mesh")

    S.carriedLetter = nil
    S.deliveryObjective = nil
    S.deliverySequenceActive = false
    S.lettersDelivered = (S.lettersDelivered or 0) + 1



    log(string.format("[PlayerState] Delivered letter %d / %d", S.lettersDelivered, S.lettersToWin or 3))

    if S.lettersDelivered >= (S.lettersToWin or 3) then
        log("[PlayerState] All letters delivered! YOU WIN")
        triggerGameWin()
    end
end

local function beginDeliverySequence(deliveryPoint, px, py, pz)
    if S.deliverySequenceActive or not S.carriedLetter then
        return
    end

    S.deliverySequenceActive = true
    S.deliveryObjective = deliveryPoint
    collectPressed = false
    hidePressed = false
    resetInputState()

    local cfg = getDeliverySequenceConfig()
    local camX, camY, camZ, lookX, lookY, lookZ, shotTag = selectDeliveryCameraShot(deliveryPoint, px, py, pz, cfg)

    log(string.format("[PlayerState] Delivery camera shot selected: %s", tostring(shotTag)))

    S.deliverySparkElapsed = 0.0
    S.deliverySparkConfig = cfg
    S.deliveryLightScale = 0.0

    ensureDeliveryLight()
    setDeliveryLight(0.0)

    -- Play SFX_DELIVER first (gear dropped), then SFX_ROBOT_INTERACT after it finishes.
    -- SFX_DELIVER is ~0.2s, so we delay SFX_ROBOT_INTERACT by 0.3s for a noticeable gap.
    if audioPlaySFX then
        audioPlaySFX(SFX_DELIVER, VOL_DELIVER)
        if setTimeout then
            setTimeout(function()
                audioPlaySFX(SFX_ROBOT_INTERACT, VOL_ROBOT_INTERACT)
            end, 0.3)
        end
    end

    if _G.StartCameraPan then
        _G.StartCameraPan({
            {
                px = camX,
                py = camY,
                pz = camZ,
                lx = lookX,
                ly = lookY,
                lz = lookZ,
            }
        }, {
            duration = cfg.panDuration,
            holdDuration = cfg.holdDuration,
            returnDuration = cfg.returnDuration,
            easing = "smoothstep",
            onComplete = completeDeliverySequence,
        })
    else
        completeDeliverySequence()
    end
end


-------------------------------------------------
-- Helper: Pick up a letter
-------------------------------------------------
local function pickupLetter(letter)
    -- Capture original world position before the entity is repositioned
    local lx, ly, lz = getPosition(letter)
    S.collectibleOriginPos = { x = lx, y = ly, z = lz }

    S.carriedLetter = letter

    if removeTag then removeTag(letter, "letter_collectible") end
    if addTag then addTag(letter, "letter_carried") end
    if audioPlay then audioPlay(letter) end
    if audioPlaySFX then audioPlaySFX(SFX_COLLECT, VOL_COLLECT) end

    disablePhysics(letter)
    if particleSystemStop then particleSystemStop(letter) end
    setVisibility(letter, false) 
    SetModel(S.player, "Frog_Anim_Gear.mesh")
    
    log("[PlayerState] Collected letter on back")
end


-------------------------------------------------
-- Pipe helpers
-------------------------------------------------

-- Returns { entrancePos, exitPos, entranceEntity } if the player is within
-- pipeEnterRadius of either end of any pipe group, nil otherwise.
-- The returned entrancePos is always the end the player is standing at,
-- so direction of travel is always away from them.
local function findNearestPipeEntrance(px, py, pz)
    local radSq = S.pipeEnterRadius * S.pipeEnterRadius
    for _, group in ipairs(PIPE_GROUPS) do
        local entrances = getEntitiesByTag("pipe_entrance_" .. group)
        local exits     = getEntitiesByTag("pipe_exit_"     .. group)
        if entrances and #entrances > 0 and exits and #exits > 0 then
            local entEnt = entrances[1]
            local extEnt = exits[1]
            local ex, ey, ez = getPosition(entEnt)
            local xx, xy, xz = getPosition(extEnt)

            -- Check entrance end
            local dex, dey, dez = px - ex, py - ey, pz - ez
            if dex*dex + dey*dey + dez*dez <= radSq then
                return {
                    entrancePos    = { x = ex, y = ey, z = ez },
                    exitPos        = { x = xx, y = xy, z = xz },
                    entranceEntity = entEnt,
                }
            end

            -- Check exit end (player enters from this side - swap direction)
            local dxx, dxy, dxz = px - xx, py - xy, pz - xz
            if dxx*dxx + dxy*dxy + dxz*dxz <= radSq then
                return {
                    entrancePos    = { x = xx, y = xy, z = xz },
                    exitPos        = { x = ex, y = ey, z = ez },
                    entranceEntity = extEnt,
                }
            end
        end
    end
    return nil
end

local function enterPipe(pipeData)
    local ep = pipeData.entrancePos
    local xp = pipeData.exitPos
    local vx = xp.x - ep.x
    local vy = xp.y - ep.y
    local vz = xp.z - ep.z
    local len = math.sqrt(vx*vx + vy*vy + vz*vz)
    if len < 0.001 then return end   -- degenerate pipe, skip

    S.inPipe          = true
    S.pipeEntrancePos = ep
    S.pipeExitPos     = xp
    S.pipeDirX        = vx / len
    S.pipeDirY        = vy / len
    S.pipeDirZ        = vz / len
    S.pipeLen         = len

    setPosition(S.player, ep.x, ep.y, ep.z)
    setVelocity(S.player, 0.0, 0.0, 0.0)
    setVisibility(S.player, false)
    if S.carriedLetter then setVisibility(S.carriedLetter, false) end
    disablePhysics(S.player)

    audioPlaySFXFromEntity(SFX_PIPE_IN, pipeData.entranceEntity, VOL_HIDE)
    log("[PlayerState] Player entered pipe")
end

local function exitPipe()
    S.inPipe = false
    setVisibility(S.player, true)

    enablePhysics(S.player)
    audioPlaySFXFromEntity(SFX_PIPE_OUT, S.player, VOL_HIDE)
    log("[PlayerState] Player exited pipe")
end

-- Expose so playerMovement.lua can trigger exit on reaching either pipe end
S.exitPipe = exitPipe


-------------------------------------------------
-- Helper: Handle hide/unhide toggle
-------------------------------------------------
local function handleHideToggle(px, py, pz)
    -- Exit pipe 
    if S.inPipe then
        local toCurX = px - S.pipeEntrancePos.x
        local toCurY = py - S.pipeEntrancePos.y
        local toCurZ = pz - S.pipeEntrancePos.z
        local currentT = toCurX * S.pipeDirX + toCurY * S.pipeDirY + toCurZ * S.pipeDirZ
        local exitThreshold = S.pipeEnterRadius
        if currentT <= exitThreshold or currentT >= S.pipeLen - exitThreshold then
            exitPipe()
            resetInputState()
        end
        return
    end

    if S.hidden then
        -- Unhide from box
        local spot = S.hiddenIn
        S.hidden = false
        S.hiddenIn = nil

        setVisibility(S.player, true)

        resetInputState()
        log("[PlayerState] Player left hiding spot")
        enablePhysics(S.player)

        if spot then
            audioPlaySFXFromEntity(SFX_HIDE_OUT, spot, VOL_HIDE)
        end
    else
        -- Check pipe entrance first, then hiding spot
        local pipeData = findNearestPipeEntrance(px, py, pz)
        if pipeData then
            enterPipe(pipeData)
            resetInputState()
            return
        end

        local bestSpot = findNearestByTag("hiding_spot", px, py, pz, S.hideRadius)
        if bestSpot then
            local bx, by, bz = getPosition(bestSpot)
            setPosition(S.player, bx, by, bz)
            setVelocity(S.player, 0.0, 0.0, 0.0)
            setVisibility(S.player, false)

            resetInputState()
            S.hidden = true
            S.hiddenIn = bestSpot
            log("[PlayerState] Player is hiding in a box")
            disablePhysics(S.player)
            -- NEW: Play hide in sound at the box location
            audioPlaySFXFromEntity(SFX_HIDE_IN, bestSpot, VOL_HIDE)
            disablePhysics(S.player)
        end
    end
end



-------------------------------------------------
-- Helper: Handle letter carry/deliver/pickup
-------------------------------------------------
local function handleLetterLogic(px, py, pz)
    -- No letter interaction while inside a pipe
    if S.inPipe then return end

    if S.deliverySequenceActive then
        return
    end

    -- If carrying a letter
    if S.carriedLetter then
        -- Keep letter on player's back
        local off = S.carriedOffset
        if S.hidden then
            local f = S.hideScaleFactor
            off = { x = off.x * f, y = off.y * f, z = off.z * f }
        end
        setPosition(S.carriedLetter, px + off.x, py + off.y, pz + off.z)

        -- Check for delivery (auto on proximity, or manual via C key / UI button)
        local deliveryPoint = findNearestByTag("letter_collection", px, py, pz, S.deliveryRadius)
        if deliveryPoint then
            collectPressed = false
            beginDeliverySequence(deliveryPoint, px, py, pz)
        end
        return  -- Don't pick up new letters while carrying
    end

    -- If hidden, skip pickup
    if S.hidden then return end

    -- Try to pick up letter (auto on proximity, or manual via C key / UI button)
    local letter = findNearestByTag("letter_collectible", px, py, pz, S.pickupRadius)
    if letter then
        collectPressed = false
        pickupLetter(letter)
    end
end



-------------------------------------------------
-- Helper: Show/hide interaction prompt billboards (UIFollowsWorldEntity)
-- Facing is automatic — the layout system projects the target's world
-- position to screen space each frame, so no rotation needed.
-------------------------------------------------
local function updateInteractionPrompts(px, py, pz)
    if not S.promptEntities or #S.promptEntities == 0 then return end

    -- While hiding or inside a pipe, suppress all prompts
    local forceHide = S.hidden or S.inPipe

    for _, prompt in ipairs(S.promptEntities) do
        local e      = prompt.entity
        local ptype  = prompt.ptype
        local target = prompt.target  -- the interactable entity this prompt follows

        if forceHide then
            setUIFollowEnabled(e, false)
        else
            local tx, ty, tz = getPosition(target)
            if tx and ty and tz then
                -- Determine radius for this prompt type
                local radius
                if ptype == "pipe" then
                    radius = S.pipeEnterRadius
                elseif ptype == "hide" then
                    radius = S.hideRadius
                else -- "collect"
                    radius = S.pickupRadius
                end

                local dx     = px - tx
                local dy     = py - ty
                local dz     = pz - tz
                local distSq = dx*dx + dy*dy + dz*dz

                setUIFollowEnabled(e, distSq <= radius * radius)
            end
        end
    end
end

-------------------------------------------------
-- Helper: Dynamically adjust the opacity of the hide_button UI element
-- based on player's proximity to hiding spots/pipes
-------------------------------------------------
local function updateHideButtonOpacity(px, py, pz)
    local hideButtons = getEntitiesByTag("hide_button")
    if not hideButtons or #hideButtons == 0 then return end

    -- Check if within range of any hide obj
    local canHide = false
    if not S.hidden and not S.inPipe then
        local pipeData = findNearestPipeEntrance(px, py, pz)
        local bestSpot = findNearestByTag("hiding_spot", px, py, pz, S.hideRadius)
        if pipeData or bestSpot then
            canHide = true
        end
    end

    local targetOpacity = 0.3
    if canHide or S.hidden or S.inPipe then
        targetOpacity = 1.0
    end

    for _, btn in ipairs(hideButtons) do
        setTextureOpacity(btn, targetOpacity)
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

    if S.deliverySequenceActive and S.deliverySparkConfig and S.deliveryObjective then
        S.deliverySparkElapsed = (S.deliverySparkElapsed or 0.0) + dt
        updateDeliverySequenceEffects(dt)
    end

    -------------------------------------------------
    -- 5. Game ended - wait for restart only
    -------------------------------------------------
    if S.gameEnded then
        if restartPressed and changeScene then
            restartPressed = false
            resetInputState()

            -- Stop Game Over loop BEFORE changeScene (script reload would lose the channel ID)
            S.stopGameOverAudio()

            -- Stop all tracked looping SFX (direct changeScene bypasses fadeOutAllTracks)
            if _G.GlobalAudio and _G.GlobalAudio.stopAllSFXChannels then
                _G.GlobalAudio.stopAllSFXChannels()
            end
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
    -- 8. Interaction prompt billboards
    -------------------------------------------------
    updateInteractionPrompts(px, py, pz)
    updateHideButtonOpacity(px, py, pz)

    -------------------------------------------------
    -- 9. Hide/Unhide logic (H key)
    -------------------------------------------------
    if hidePressed then
        hidePressed = false
        handleHideToggle(px, py, pz)
    end

    -------------------------------------------------
    -- 10. Letter carry/deliver/pickup logic (C key)
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

function S.isInPipe()
    return S.inPipe == true
end

function S.canBeCaught()
    -- cannot be caught while hiding or inside a pipe
    if S.hidden or S.inPipe then
        return false
    end

    if S.deliverySequenceActive or (_G.CameraPan and _G.CameraPan.active) then
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

    -- If player had the gear, keep it on their back and respawn at the gear's origin.
    -- If they hadn't collected it yet, fall through to normal checkpoint/start respawn.
    if S.carriedLetter then
        -- Do NOT drop the gear — leave carriedLetter, letter_carried tag, and
        -- Frog_Anim_Gear.mesh all intact. Respawn player at gear's original position.
        local originPos = S.collectibleOriginPos or S.startPos
        if originPos then
            S.pendingRespawn = {
                entity = player,
                x = originPos.x,
                y = originPos.y,
                z = originPos.z,
            }
        end
        log("[PlayerState] Died with gear — respawning at gear origin, gear retained")
    else
        -- No gear collected: respawn at checkpoint or start as usual
        local respawn = S.checkpointPos or S.startPos
        if respawn then
            -- just store, dont teleport yet cos gives mem leak
            S.pendingRespawn = {
                entity = player,
                x = respawn.x,
                y = respawn.y,
                z = respawn.z,
            }
        end
    end

    -- 1 second of invincibility
    S.respawnCooldown = 1.0
end

-- tick cooldown
registerUpdate(function(dt)
    S.update(dt)
    -- print("[Game_PlayerState] runtime _G.CurrentLevelName=", _G.CurrentLevelName)
end)
