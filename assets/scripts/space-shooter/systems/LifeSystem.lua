-- ============================================================================
-- LifeSystem.lua - Server-Authoritative Life Management
-- ============================================================================
-- CRITICAL: This system MUST only run on authoritative instances (Server/Solo).
-- Clients NEVER modify life values - they wait for ENTITY_UPDATE from server.
-- 
-- Why? To prevent cheating and ensure game state consistency:
-- - Client cannot give themselves infinite health
-- - Server is the single source of truth for life/death
-- - All clients see the same state
-- ============================================================================

local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")
local LifeSystem = {}

function LifeSystem.init()
    print("[LifeSystem] Initialized (hasAuthority: " .. tostring(ECS.capabilities.hasAuthority) .. ")")
end

function LifeSystem.update(dt)
    -- ⚠️ AUTHORITY CHECK: Only server/solo can modify life
    if not ECS.capabilities.hasAuthority then return end
    if not ECS.isGameRunning then return end

    local entities = ECS.getEntitiesWith({"Life", "Transform"})

    for _, id in ipairs(entities) do
        local life = ECS.getComponent(id, "Life")
        local t = ECS.getComponent(id, "Transform")

        -- Boundary Check (Optimization): Destroy entities that go too far off-screen
        if t.x < -50 or t.x > 50 or t.y < -30 or t.y > 30 then
            life.amount = 0
        end

        if life.invulnerableTime and life.invulnerableTime > 0 then
            life.invulnerableTime = math.max(0, life.invulnerableTime - dt)
            -- Skip death while invulnerable
        else
            if life.amount <= 0 then
                local player = ECS.getComponent(id, "Player")
                local enemy = ECS.getComponent(id, "Enemy")

                -- In multiplayer server mode, broadcast enemy death to clients
                if enemy and ECS.capabilities.hasNetworkSync and not life.deathEventSent then
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

                -- In Solo Mode (Authority + Rendering), spawn explosion locally
                if enemy and ECS.capabilities.hasRendering then
                    Spawns.createExplosion(t.x, t.y, t.z)
                end

                local finalScore = nil

                if player then
                    print("DEBUG: Player Dead. Life Amount: " .. life.amount)
                    local scoreEntities = ECS.getEntitiesWith({"Score"})
                    finalScore = 0
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

                -- Notify clients or trigger local game over
                if ECS.capabilities.hasNetworkSync then
                    -- Multiplayer server: Broadcast entity destruction
                    ECS.broadcastNetworkMessage("ENTITY_DESTROY", id)
                elseif ECS.capabilities.hasLocalMode and player and finalScore then -- Changed from isServer() to capabilities.isLocalMode
                    -- Solo mode: Trigger local game over
                    ECS.sendMessage("GAME_OVER", tostring(finalScore))
                end
            end
        end
    end
end

ECS.registerSystem(LifeSystem)