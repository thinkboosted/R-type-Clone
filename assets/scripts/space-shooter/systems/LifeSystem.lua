local LifeSystem = {}

function LifeSystem.init()
    print("[LifeSystem] Initialized")
end

function LifeSystem.update(dt)
    if not ECS.isServer() and not ECS.isLocal then return end
    if not ECS.isGameRunning then return end

    local entities = ECS.getEntitiesWith({"Life"})

    for _, id in ipairs(entities) do
        local life = ECS.getComponent(id, "Life")

        if life.invulnerableTime and life.invulnerableTime > 0 then
            life.invulnerableTime = math.max(0, life.invulnerableTime - dt)
            -- Skip death while invulnerable
        else
            if life.amount <= 0 then
                local player = ECS.getComponent(id, "Player")
                local enemy = ECS.getComponent(id, "Enemy")

                if enemy and ECS.isServer() and not life.deathEventSent then
                    local t = ECS.getComponent(id, "Transform")
                    local phys = ECS.getComponent(id, "Physic")
                    local vx, vy, vz = 0, 0, 0
                    if phys then
                        vx, vy, vz = phys.vx or 0, phys.vy or 0, phys.vz or 0
                    end
                    if t then
                        local msg = string.format("%s %f %f %f %f %f %f", id, t.x, t.y, t.z, vx, vy, vz)
                        ECS.broadcastNetworkMessage("ENEMY_DEAD", msg)
                        life.deathEventSent = true
                    end
                end

                if player then
                    print("DEBUG: Player Dead. Life Amount: " .. life.amount)
                    local scoreEntities = ECS.getEntitiesWith({"Score"})
                    local finalScore = 0
                    if #scoreEntities > 0 then
                        local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
                        finalScore = scoreComp.value
                    end
                    print("GAME OVER: Player died! Final Score: " .. finalScore)

                    local netId = ECS.getComponent(id, "NetworkId")
                    if netId then
                        ECS.sendToClient(tonumber(netId.id), "GAME_OVER", tostring(finalScore))
                        ECS.sendMessage("SERVER_PLAYER_DEAD", tostring(netId.id))
                    end
                end

                ECS.destroyEntity(id)

                if ECS.isServer() then
                    ECS.broadcastNetworkMessage("ENTITY_DESTROY", id)
                elseif ECS.isLocal and player then
                    ECS.sendMessage("GAME_OVER", tostring(finalScore))
                end
            end
        end
    end
end

ECS.registerSystem(LifeSystem)