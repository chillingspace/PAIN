local registered = false

registerUpdate(function(dt)
    if registered then return end

    local x, y, z = getPosition(entityId)
    _G.UI = _G.UI or {
        hearts = {}
    }

    table.insert(_G.UI.hearts, { id = entityId, x = x })

    log(string.format(
        "[registerHeart] registered heart id=%s x=%.3f (count=%d)",
        tostring(entityId),
        x or 0.0,
        #_G.UI.hearts
    ))

    registered = true
end)
