local LifeSystem = {}

function LifeSystem.init()
    print("[LifeSystem] Initialized")
end

function LifeSystem.update(dt)
    local entities = ECS.getEntitiesWith({"Life"})

    for _, id in ipairs(entities) do
        local life = ECS.getComponent(id, "Life")
        if life.amount <= 0 then
            -- Check if it's the player
            local player = ECS.getComponent(id, "Player")
            if player then
                -- Get final score
                local scoreEntities = ECS.getEntitiesWith({"Score"})
                local finalScore = 0
                if #scoreEntities > 0 then
                    local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
                    finalScore = scoreComp.value
                end
                print("GAME OVER: Player died!")
                print("Final Score: " .. finalScore)
            end

            ECS.destroyEntity(id)
            
            if ECS.isServer() then
                ECS.broadcastNetworkMessage("ENTITY_DESTROY", id)
            end

        end
    end
end

ECS.registerSystem(LifeSystem)
