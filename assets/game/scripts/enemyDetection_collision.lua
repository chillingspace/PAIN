
-- collision-based (enemy detection) script

local player = nil
local minimapTagged = false

registerUpdate(function(dt)
    if (not minimapTagged) and addTag then
        addTag(entityId, "Enemy")
        addTag(entityId, "danger")
        addTag(entityId, "danger_collision")
        minimapTagged = true
    end

    if not player then
        player = findEntity("Player")
        if not player then 
            return 
        end

        if PlayerState and PlayerState.init then
            PlayerState.init(player)
        end
    end
end)

registerOnCollision(function(self, other)
    -- only care if the player got collided w
    if not player then
        player = findEntity("Player")
        if not player then 
            return 
        end
    end

     -- ignore collisions during respawn invincibility
    if PlayerState and PlayerState.canBeCaught and not PlayerState.canBeCaught() then
        return
    end

    if other == player or self == player then
        log("[enemyDetection_collision] Player collided with enemy/trigger")

        if PlayerState and PlayerState.onCaught then
            PlayerState.onCaught(player)
        end
    end
end, player)
