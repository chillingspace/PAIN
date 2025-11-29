
-- Test_AI_CommandQueue.lua
-- Just spams commands to see if queue handles volume, no crashes

local M = {}
local timer = 0.0

function M.update(ctx, dt)
    timer = timer + dt
    if timer < 1.0 then return end
    timer = 0.0

    for i = 1, 20 do
        ctx.ai:set_move_target(i, 0, i)    -- nonsense positions
        ctx.ai:play_anim("Run")
        ctx.ai:clear_move_target()
    end

    log("[CMD] pushed 60 commands")
end

return M
