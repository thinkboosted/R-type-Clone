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

local function getFinalScore()
    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities > 0 then
        local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
        if scoreComp and scoreComp.value then
            return scoreComp.value
        end
    end
    return 0
end

local function showLocalGameOver(finalScore)
    if _G.MenuSystem and _G.MenuSystem.showDeathScreen then
        _G.MenuSystem.showDeathScreen(finalScore)
    end
    ECS.sendMessage("GAME_OVER", tostring(finalScore))
end

local function handlePlayerDeath(id, life, transform)
    if life.deathEventSent then
        return true
    end

    life.deathEventSent = true
    ECS.deathSlowdownActive = true
    ECS.timeScale = 1.0
    ECS.isGameRunning = false

    local finalScore = getFinalScore()
    print("GAME OVER: Player died! Final Score: " .. finalScore)

    if ECS.capabilities.hasRendering and transform then
        Spawns.createExplosion(transform.x, transform.y, transform.z)
    end

    if not ECS.capabilities.hasNetworkSync then
        ECS.sendMessage("MusicStop", "bgm")
        ECS.sendMessage("SoundPlay", "gameover:effects/gameover.wav:" .. ECS.getSfxVolume(100))
        showLocalGameOver(finalScore)
    else
        ECS.broadcastNetworkMessage("STOP_MUSIC", "bgm")
        ECS.broadcastNetworkMessage("PLAY_SOUND", "gameover:effects/gameover.wav:100")

        local ownerClientId = nil
        local netIdentity = ECS.getComponent(id, "NetworkIdentity")
        if netIdentity and netIdentity.ownerId and tonumber(netIdentity.ownerId) then
            ownerClientId = tonumber(netIdentity.ownerId)
        else
            local legacyNetId = ECS.getComponent(id, "NetworkId")
            if legacyNetId and legacyNetId.id and tonumber(legacyNetId.id) then
                ownerClientId = tonumber(legacyNetId.id)
            end
        end

        if ownerClientId and ownerClientId > 0 then
            ECS.sendToClient(ownerClientId, "GAME_OVER", tostring(finalScore))
            ECS.sendMessage("SERVER_PLAYER_DEAD", tostring(ownerClientId))
        end

        if ECS.capabilities.hasRendering then
            showLocalGameOver(finalScore)
        end
    end

    return true
end

function LifeSystem.init()
    print("[LifeSystem] Initialized (hasAuthority: " .. tostring(ECS.capabilities.hasAuthority) .. ")")
end

function LifeSystem.update(dt)
    -- Allow running on clients for visual effects (explosions), but authority logic only on server/solo
    if not ECS.capabilities.hasAuthority and not ECS.isGameRunning then return end
    if ECS.isPaused then return end

    local entities = ECS.getEntitiesWith({"Life", "Transform"})

    for _, id in ipairs(entities) do
        local life = ECS.getComponent(id, "Life")
        local t = ECS.getComponent(id, "Transform")
        local hasAuthority = ECS.hasComponent(id, "ServerAuthority")
        local player = ECS.getComponent(id, "Player")
        local enemy = ECS.getComponent(id, "Enemy")

        -- Boundary Check (Optimization): Destroy entities that go too far off-screen (only on authority)
        if hasAuthority and (t.x < -50 or t.x > 50 or t.y < -30 or t.y > 30) then
            life.amount = 0
        end

        if player and life.amount <= 0 and (hasAuthority or ECS.capabilities.hasAuthority) then
            handlePlayerDeath(id, life, t)
            -- Keep the destroyed ship body out of gameplay, but the death UI is already drawn.
            ECS.destroyEntity(id)
            return
        end

        if life.invulnerableTime and life.invulnerableTime > 0 then
            life.invulnerableTime = math.max(0, life.invulnerableTime - dt)

        -- Skip death while invulnerable (for all entities)
        if life.invulnerableTime <= 0 then
            if ECS.hasComponent(id, "Player") then
                local color = ECS.getComponent(id, "Color")
                    if color then
                        color.r = 1.0
                        color.g = 1.0
                        color.b = 1.0
                        ECS.addComponent(id, "Color", color) -- color back to white after hit
                    end
                end
            end
        else
            if life.amount <= 0 then
                -- Authority-required logic (damage, broadcasts, etc.) only for authoritative entities
                if hasAuthority then
                    -- In multiplayer server mode, broadcast enemy death to clients
                    if enemy and ECS.capabilities.hasNetworkSync and not life.deathEventSent then
                        -- Don't broadcast for boundary deaths
                        if not (t.x < -50 or t.x > 50 or t.y < -30 or t.y > 30) then
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
                    end

                -- In Solo Mode (Authority + Rendering), spawn explosion locally
                if enemy and ECS.capabilities.hasRendering then
                    -- Don't spawn explosion for boundary deaths
                    if not (t.x < -50 or t.x > 50 or t.y < -30 or t.y > 30) then
                        Spawns.createExplosion(t.x, t.y, t.z)
                        
                        -- Play enemy death sound
                        if not ECS.capabilities.hasNetworkSync then
                            ECS.sendMessage("SoundPlay", "enemy_death_" .. id .. ":effects/explosion.wav:" .. ECS.getSfxVolume(90))
                        end
                    end
                end

                end  -- Close the if hasAuthority

                -- Destroy entity for all (authoritative and non-authoritative)
                ECS.destroyEntity(id)

                -- Notify clients or trigger local game over (only for authoritative deaths)
                if hasAuthority then
                    if ECS.capabilities.hasNetworkSync then
                        -- Multiplayer server: Broadcast entity destruction
                        ECS.broadcastNetworkMessage("ENTITY_DESTROY", id)
                    end
                end
            end
        end
    end
end

ECS.registerSystem(LifeSystem)
