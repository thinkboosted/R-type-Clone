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
                print("GAME OVER: Player died! Final Score: " .. finalScore)
                
                -- Send Game Over to Client
                -- We need the ClientID. In NetworkSystem spawn, we attached "NetworkId" component?
                -- Let's assume we can get it or just broadcast if simple
                -- For now, broadcast GAME_OVER {Score} might kill everyone's game if logic is generic?
                -- We should target.
                -- Let's try to find the client ID from NetworkSystem map if accessible, or add NetworkId component.
                -- Added "NetworkId" component in NetworkSystem.spawnPlayerForClient.
                
                local netId = ECS.getComponent(id, "NetworkId")
                if netId then
                    ECS.sendToClient(tonumber(netId.id), "GAME_OVER", tostring(finalScore))
                end
            end

            ECS.destroyEntity(id)
            
            if ECS.isServer() then
                ECS.broadcastNetworkMessage("ENTITY_DESTROY", id)
            end

        end
    end
end

ECS.registerSystem(LifeSystem)
