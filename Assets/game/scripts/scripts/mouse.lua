
registerUpdate(function(dt)
    local id = entityId
    local px, py = get2DPosition(id)  -- your 2D pos
    local mx, my = getMousePos()      -- screen mouse

    -- Direct screen-to-world (no scaling needed if coords match)
    local dx = mx - px
    local dy = my - py

    set2DPosition(id, px + dx, py + dy)
end)
