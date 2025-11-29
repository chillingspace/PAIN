
-- AI_patrol_and_investigate.lua 
-- States:
--   0 = PATROL
--   1 = INVESTIGATE (go to last heard noise, wait a bit)
--   2 = CHASE (visible enemy)

-- local M = {}

-- -- ----------------------------------------
-- -- Helpers
-- -- ----------------------------------------

-- local STATE_PATROL      = 0
-- local STATE_INVESTIGATE = 1
-- local STATE_CHASE       = 2

-- -- Patrol square around center with fixed offsets
-- local function pick_next_patrol_wp(ctx)
--     local idx = ctx.bb:get_number("patrol_idx", 0)
--     idx = (idx + 1) % 4
--     ctx.bb:set_number("patrol_idx", idx)

--     local center = ctx.bb:get_vec3("patrol_center", 0, 0, 0)

--     local offsets = {
--         { x =  4, y = 0, z =  4 },
--         { x = -4, y = 0, z =  4 },
--         { x = -4, y = 0, z = -4 },
--         { x =  4, y = 0, z = -4 },
--     }

--     local o = offsets[idx + 1]
--     local x = center.x + o.x
--     local y = center.y + o.y
--     local z = center.z + o.z
--     return x, y, z
-- end

-- -- Checks if we are close enough to a target position
-- local function is_close_to_target(ctx, target, radius)
--     local selfpos = ctx.bb:get_vec3("selfpos", 0, 0, 0)
--     local dx = target.x - selfpos.x
--     local dz = target.z - selfpos.z
--     local distSq = dx * dx + dz * dz
--     return distSq <= (radius * radius)
-- end

-- -- Convenience: set a new move target only if it changed
-- local function set_move_target_if_changed(ctx, key, x, y, z)
--     local old = ctx.bb:get_vec3(key, 1e30, 1e30, 1e30) -- absurd default
--     if old.x ~= x or old.y ~= y or old.z ~= z then
--         ctx.bb:set_vec3(key, x, y, z)
--         ctx.ai:set_move_target(x, y, z)
--     end
-- end

-- -- ----------------------------------------
-- -- Main update
-- -- ----------------------------------------

-- function M.update(ctx, dt)
--     -- Current state (default: patrol)
--     local state = ctx.bb:get_number("state", STATE_PATROL)

--     -- ======================================================
--     -- 1) Immediate noise reaction (highest priority)
--     -- ======================================================
--     local noisePos = ctx.ai:get_last_noise_pos()
--     if noisePos then
--         -- Switch to INVESTIGATE state on fresh noise
--         state = STATE_INVESTIGATE
--         ctx.bb:set_number("state", state)

--         ctx.bb:set_vec3("investigatePos", noisePos.x, noisePos.y, noisePos.z)
--         ctx.bb:set_number("investigate_t", 0.0)

--         set_move_target_if_changed(ctx, "current_move_target",
--             noisePos.x, noisePos.y, noisePos.z)
--         ctx.ai:play_anim("Alert")

--         -- For debugging:
--         -- log("[AI] INVESTIGATE: heard noise at ", noisePos.x, noisePos.z)

--         return
--     end

--     -- ======================================================
--     -- 2) If we see an enemy, CHASE (next priority)
--     --    Note: this overrides patrol but not an active noise.
--     -- ======================================================
--     local vis = ctx.ai:get_visible_enemies()
--     if #vis > 0 then
--         local targetId = vis[1]
--         ctx.bb:set_u32("targetId", targetId)

--         local pos = ctx.ai:get_entity_pos(targetId)
--         if pos then
--             state = STATE_CHASE
--             ctx.bb:set_number("state", state)

--             set_move_target_if_changed(ctx, "current_move_target",
--                 pos.x, pos.y, pos.z)
--             ctx.ai:play_anim("Run")

--             -- For debugging:
--             -- log("[AI] CHASE: enemy=", targetId, " target=", pos.x, pos.z)

--             return
--         end
--     end

--     -- ======================================================
--     -- 3) Handle current state
--     -- ======================================================

--     -- ---------- INVESTIGATE ----------
--     if state == STATE_INVESTIGATE then
--         local t = ctx.bb:get_number("investigate_t", 0.0) + dt
--         ctx.bb:set_number("investigate_t", t)

--         local ipos = ctx.bb:get_vec3("investigatePos", 0, 0, 0)

--         -- Move to investigate position
--         set_move_target_if_changed(ctx, "current_move_target",
--             ipos.x, ipos.y, ipos.z)
--         ctx.ai:play_anim("Alert")

--         -- Optional: if we are close enough, just stand and keep "looking around"
--         local close_radius = 0.5
--         if is_close_to_target(ctx, ipos, close_radius) then
--             -- we arrived: stop movement but keep alert for a while
--             ctx.ai:clear_move_target()
--         end

--         -- After 3 seconds, give up and go back to PATROL
--         if t > 3.0 then
--             ctx.bb:set_bool("investigating", false)
--             ctx.bb:set_number("state", STATE_PATROL)
--             ctx.ai:play_anim("Idle")
--             -- Do not return: fall through next frame as PATROL
--         end

--         return
--     end

--     -- ---------- CHASE ----------
--     if state == STATE_CHASE then
--         -- If we lost our target (no vis anymore above), fall back to patrol.
--         -- We already checked vis earlier and found none, so just:
--         ctx.bb:set_number("state", STATE_PATROL)
--         ctx.ai:play_anim("Idle")
--         ctx.ai:clear_move_target()

--         -- For debugging:
--         -- log("[AI] CHASE: lost target, reverting to PATROL")

--         -- Continue this frame as patrol logic below
--         -- no 'return' here
--     end

--     -- ---------- PATROL ----------
--     -- Default state: walk around a square

--     local patrol_t = ctx.bb:get_number("patrol_t", 0.0) + dt
--     local patrol_interval = 1.5  -- how often to pick next corner
--     local reached_radius = 0.3   -- how close is "at patrol point"

--     -- Current patrol target (so we can check distance to it)
--     local patrol_wp = ctx.bb:get_vec3("patrol_wp", 1e30, 1e30, 1e30)
--     local has_wp = (patrol_wp.x ~= 1e30)

--     if has_wp then
--         -- If we are close enough to current waypoint, we can idle until interval
--         if is_close_to_target(ctx, patrol_wp, reached_radius) then
--             ctx.ai:clear_move_target()
--             ctx.ai:play_anim("Idle")
--         else
--             -- Ensure we keep moving towards wp
--             set_move_target_if_changed(ctx, "current_move_target",
--                 patrol_wp.x, patrol_wp.y, patrol_wp.z)
--             ctx.ai:play_anim("Walk")
--         end
--     end

--     -- Every patrol_interval seconds, pick a NEW patrol waypoint
--     if patrol_t >= patrol_interval or not has_wp then
--         patrol_t = 0.0
--         local x, y, z = pick_next_patrol_wp(ctx)
--         ctx.bb:set_vec3("patrol_wp", x, y, z)

--         set_move_target_if_changed(ctx, "current_move_target", x, y, z)
--         ctx.ai:play_anim("Walk")

--         -- For debugging:
--         -- log("[AI] PATROL: new waypoint = ", x, z)
--     end

--     ctx.bb:set_number("patrol_t", patrol_t)
--     ctx.bb:set_number("state", STATE_PATROL)
-- end

-- return M







-- for now,
-- use direct setVelocity / getPosition (no set_move_target)


local M = {}

-- ----------------------------------------
-- Constants / state IDs
-- ----------------------------------------

local STATE_PATROL      = 0
local STATE_INVESTIGATE = 1
local STATE_CHASE       = 2

local patrolSpeed      = 3.5
local chaseSpeed       = 5.0
local investigateSpeed = 4.0

local patrolArrivalRadius      = 0.4
local investigateArrivalRadius = 0.5
local chaseStopRadius          = 0.6
local investigateDuration      = 3.0

-- ----------------------------------------
-- Helpers
-- ----------------------------------------

local function get_self(ctx)
    local selfId = ctx.ai:get_self_id()
    if selfId == 0 then return nil, nil end
    local sx, sy, sz = getPosition(selfId)
    return selfId, { x = sx, y = sy, z = sz }
end

local function distance_sq(a, b)
    local dx = b.x - a.x
    local dz = b.z - a.z
    return dx * dx + dz * dz, dx, dz
end

-- Move self towards target using setVelocity.
-- Returns true if we are within 'radius' of the target.
local function move_towards(selfId, selfPos, targetPos, speed, radius)
    local distSq, dx, dz = distance_sq(selfPos, targetPos)

    if distSq <= radius * radius then
        -- Reached: stop horizontal motion
        setVelocity(selfId, 0.0, 0.0, 0.0)
        return true
    end

    local dist = math.sqrt(distSq)
    if dist < 1e-4 then
        setVelocity(selfId, 0.0, 0.0, 0.0)
        return true
    end

    dx = dx / dist
    dz = dz / dist

    setVelocity(selfId, dx * speed, 0.0, dz * speed)
    return false
end

-- Patrol square around center with fixed offsets
local function pick_next_patrol_wp(ctx, selfPos)
    local idx = ctx.bb:get_number("patrol_idx", 0)
    idx = (idx + 1) % 4
    ctx.bb:set_number("patrol_idx", idx)

    -- default patrol center = current position on first call
    local center = ctx.bb:get_vec3("patrol_center", selfPos.x, selfPos.y, selfPos.z)
    ctx.bb:set_vec3("patrol_center", center.x, center.y, center.z)

    local offsets = {
        { x =  4, y = 0, z =  4 },
        { x = -4, y = 0, z =  4 },
        { x = -4, y = 0, z = -4 },
        { x =  4, y = 0, z = -4 },
    }

    local o = offsets[idx + 1]
    return {
        x = center.x + o.x,
        y = center.y + o.y,
        z = center.z + o.z
    }
end

-- ----------------------------------------
-- Main update
-- ----------------------------------------

function M.update(ctx, dt)
    local selfId, selfPos = get_self(ctx)
    if not selfId then return end

    -- keep selfpos mirrored into blackboard for debugging
    ctx.bb:set_vec3("selfpos", selfPos.x, selfPos.y, selfPos.z)

    local state = ctx.bb:get_number("state", STATE_PATROL)

    -- ======================================================
    -- 1) Noise: enter INVESTIGATE
    -- ======================================================
    local noisePos = ctx.ai:get_last_noise_pos()
    if noisePos then
        state = STATE_INVESTIGATE
        ctx.bb:set_number("state", state)
        ctx.bb:set_vec3("investigatePos", noisePos.x, noisePos.y, noisePos.z)
        ctx.bb:set_number("investigate_t", 0.0)

        ctx.ai:play_anim("Alert")
        return
    end

    -- ======================================================
    -- 2) Visible enemy: CHASE
    -- ======================================================
    local vis = ctx.ai:get_visible_enemies()
    if #vis > 0 then
        local targetId = vis[1]
        ctx.bb:set_u32("targetId", targetId)

        local tpos = ctx.ai:get_entity_pos(targetId)
        if tpos then
            state = STATE_CHASE
            ctx.bb:set_number("state", state)

            local targetPos = { x = tpos.x, y = tpos.y, z = tpos.z }
            ctx.ai:play_anim("Run")

            local reached = move_towards(selfId, selfPos, targetPos, chaseSpeed, chaseStopRadius)
            if reached then
                ctx.ai:play_anim("Idle")
            end
            return
        end
    end

    -- ======================================================
    -- 3) State-specific behaviour
    -- ======================================================

    -- ---------- INVESTIGATE ----------
    if state == STATE_INVESTIGATE then
        local t = ctx.bb:get_number("investigate_t", 0.0) + dt
        ctx.bb:set_number("investigate_t", t)

        local ipos = ctx.bb:get_vec3("investigatePos", selfPos.x, selfPos.y, selfPos.z)
        local targetPos = { x = ipos.x, y = ipos.y, z = ipos.z }

        ctx.ai:play_anim("Alert")
        move_towards(selfId, selfPos, targetPos, investigateSpeed, investigateArrivalRadius)

        if t > investigateDuration then
            -- Done investigating, back to patrol
            ctx.bb:set_number("state", STATE_PATROL)
            ctx.ai:play_anim("Idle")
        end

        return
    end

    -- ---------- PATROL ----------
    -- Default: patrol around a square
    local patrol_t = ctx.bb:get_number("patrol_t", 0.0) + dt
    local patrol_interval = 1.5

    local patrol_wp_v = ctx.bb:get_vec3("patrol_wp", 1e30, 1e30, 1e30)
    local has_wp = (patrol_wp_v.x ~= 1e30)

    if not has_wp then
        -- pick initial waypoint
        local wp = pick_next_patrol_wp(ctx, selfPos)
        ctx.bb:set_vec3("patrol_wp", wp.x, wp.y, wp.z)
        patrol_wp_v = wp
        has_wp = true
        patrol_t = 0.0
    end

    local patrol_wp = { x = patrol_wp_v.x, y = patrol_wp_v.y, z = patrol_wp_v.z }

    ctx.ai:play_anim("Walk")
    local reached = move_towards(selfId, selfPos, patrol_wp, patrolSpeed, patrolArrivalRadius)

    if reached or patrol_t >= patrol_interval then
        -- choose next corner
        local new_wp = pick_next_patrol_wp(ctx, selfPos)
        ctx.bb:set_vec3("patrol_wp", new_wp.x, new_wp.y, new_wp.z)
        patrol_t = 0.0
    end

    ctx.bb:set_number("patrol_t", patrol_t)
    ctx.bb:set_number("state", STATE_PATROL)
end

return M
