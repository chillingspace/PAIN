local registered = false

registerUpdate(function(dt)
    if registered then return end

    local x, y, z = getPosition(entityId)
    _G.UI = _G.UI or {
        hearts = {}
    }

    table.insert(_G.UI.hearts, { id = entityId, x = x })
    registered = true
end)
