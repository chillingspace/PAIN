
-- Test_AI_Self.lua

local M = {}

function M.update(ctx, dt)
    local selfId = ctx.ai:get_self_id()
    local sx, sy, sz = getPosition(selfId)
    local spos = ctx.ai:get_self_pos()

    log("[SELF] id=", selfId,
        " getPosition=", sx, sy, sz,
        " get_self_pos=", spos.x, spos.y, spos.z)
end

return M
