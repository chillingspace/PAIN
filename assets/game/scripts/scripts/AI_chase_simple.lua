
-- AI_chase_simple.lua
-- if something is visible, run at it, else idle

-- local M = {}

-- function M.update(ctx, dt)
--     local vis = ctx.ai:get_visible_enemies()

--     if #vis > 0 then
--         local targetId = vis[1]
--         ctx.bb:set_u32("targetId", targetId)

--         local tpos = ctx.ai:get_entity_pos(targetId)
--         if tpos then

            
--             -- Optional: log positions for debugging
--             local selfpos = ctx.bb:get_vec3("selfpos", 0, 0, 0)
--             log("[AI] chasing id=", targetId,
--                 " self=", selfpos.x, selfpos.z,
--                 " target=", tpos.x, tpos.z)

--             ctx.ai:play_anim("Run")
--             ctx.ai:set_move_target(tpos.x, tpos.y, tpos.z)
--         else
--             ctx.ai:play_anim("Idle")
--             ctx.ai:clear_move_target()
--         end
--     else
--         ctx.ai:play_anim("Idle")
--         ctx.ai:clear_move_target()
--     end
-- end

-- return M




local M = {}
local moveSpeed = 4.0
local stopRadius = 0.5

function M.update(ctx, dt)
    local selfId = ctx.ai:get_self_id()
    if selfId == 0 then return end

    local vis = ctx.ai:get_visible_enemies()
    if #vis == 0 then
        ctx.ai:play_anim("Idle")
        setVelocity(selfId, 0.0, 0.0, 0.0)
        return
    end

    local targetId = vis[1]
    local sx, sy, sz = getPosition(selfId)
    local tpos = ctx.ai:get_entity_pos(targetId)
    if not tpos then
        ctx.ai:play_anim("Idle")
        setVelocity(selfId, 0.0, 0.0, 0.0)
        return
    end

    local dx = tpos.x - sx
    local dz = tpos.z - sz
    local distSq = dx*dx + dz*dz

    if distSq < stopRadius * stopRadius then
        ctx.ai:play_anim("Idle")
        setVelocity(selfId, 0.0, 0.0, 0.0)
        return
    end

    local dist = math.sqrt(distSq)
    dx = dx / dist
    dz = dz / dist

    ctx.ai:play_anim("Run")
    setVelocity(selfId, dx * moveSpeed, 0.0, dz * moveSpeed)
end

return M
