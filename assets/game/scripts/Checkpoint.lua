
-- Checkpoint.lua

local player = nil

registerUpdate(function(dt)
    if not player then
        player = findEntity("Player")
    end
end)

registerOnCollision(function(self, other)
    if not player then
        player = findEntity("Player")
        if not player then return end
    end

    if other == player or self == player then
        local px, py, pz = getPosition(player)
        -- log("[Checkpoint] player reached checkpoint at:", px, py, pz)
        
        if PlayerState and PlayerState.setCheckpoint then
            PlayerState.setCheckpoint(player, self)
        end
    end
end)
