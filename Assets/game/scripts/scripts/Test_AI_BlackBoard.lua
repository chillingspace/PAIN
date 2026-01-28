
-- Test_AI_BlackBoard.lua

local M = {}

function M.update(ctx, dt)
    -- write
    ctx.bb:set_number("num_test", 42.5)
    ctx.bb:set_bool("bool_test", true)
    ctx.bb:set_vec3("vec_test", 1.0, 2.0, 3.0)
    ctx.bb:set_u32("u32_test", 1234)

    -- read back
    local n  = ctx.bb:get_number("num_test", -1)
    local b  = ctx.bb:get_bool("bool_test", false)
    local v  = ctx.bb:get_vec3("vec_test", 0, 0, 0)
    local id = ctx.bb:get_u32("u32_test", 0)

    log("[BB] num=", n, " bool=", b,
        " vec=", v.x, v.y, v.z,
        " u32=", id)
end

return M
