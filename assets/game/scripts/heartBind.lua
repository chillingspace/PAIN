local registered = false

registerUpdate(function(dt)
    if registered then return end

    local x = getPosition(entityId)
    _G.UI = _G.UI or {}
    _G.UI.hearts = _G.UI.hearts or {}

    table.insert(_G.UI.hearts, { id = entityId, x = x })
    registered = true
end)
