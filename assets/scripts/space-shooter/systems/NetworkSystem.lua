-- Expose globally so other systems (MenuSystem) can read connection state.
local NetworkSystem = {}
_G.NetworkSystem = NetworkSystem

NetworkSystem.clientEntities = {}
NetworkSystem.serverEntities = {}
NetworkSystem.myServerId = nil
NetworkSystem.deathAnims = {}

NetworkSystem.broadcastTimer = 0
NetworkSystem.broadcastInterval = 0.08
NetworkSystem.readyClients = {}
NetworkSystem.tickCounter = 0
NetworkSystem.debugAccum = 0
NetworkSystem.debugSentPlayers = 0
NetworkSystem.debugSentBullets = 0
NetworkSystem.debugSentEnemies = 0
NetworkSystem.debugSentScores = 0

local function hasActiveClients()
    return next(NetworkSystem.clientEntities) ~= nil
end

local function hasReadyClients()
    return next(NetworkSystem.readyClients) ~= nil
end

local function extractVelocity(phys)
    if not phys then return 0, 0, 0 end
    return phys.vx or 0, phys.vy or 0, phys.vz or 0
end

local function formatStateMessage(id, transform, phys, typeNum)
    local vx, vy, vz = extractVelocity(phys)
    return string.format("%s %f %f %f %f %f %f %f %f %f %d", id, transform.x, transform.y, transform.z, transform.rx, transform.ry, transform.rz, vx, vy, vz, typeNum)
end

local function countTableKeys(tbl)
    local c = 0
    for _ in pairs(tbl) do c = c + 1 end
    return c
end

function NetworkSystem.resetWorldState()
    -- Clear lingering gameplay entities so a new client does not spawn into stale hazards.
    local function destroyAndBroadcast(entities)
        for _, id in ipairs(entities) do
            ECS.destroyEntity(id)
            ECS.broadcastNetworkMessage("ENTITY_DESTROY", id)
            ECS.sendMessage("PhysicCommand", "DestroyBody:" .. id .. ";")
        end
    end

    destroyAndBroadcast(ECS.getEntitiesWith({"Enemy"}))
    destroyAndBroadcast(ECS.getEntitiesWith({"Bullet"}))
    destroyAndBroadcast(ECS.getEntitiesWith({"Bonus"}))

    NetworkSystem.deathAnims = {}

    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities > 0 then
        local s = ECS.getComponent(scoreEntities[1], "Score")
        s.value = 0
    end

    ECS.sendMessage("RESET_DIFFICULTY", "")
end

function NetworkSystem.init()
    if ECS.isServer() then
        print("[NetworkSystem] Server Mode")

        -- Create Logic Score Entity
        local scoreEnt = ECS.createEntity()
        ECS.addComponent(scoreEnt, "Score", Score(0))

        ECS.subscribe("ClientConnected", function(msg)
            local clientId = string.match(msg, "^(%d+)")
            print("Client Connected: " .. clientId .. " (Auto-Spawning)")

            -- If this is the first/only client, wipe stale enemies/bullets so they don't collide instantly.
            if not hasActiveClients() then
                NetworkSystem.resetWorldState()
            end

            if not NetworkSystem.clientEntities[clientId] then
                NetworkSystem.spawnPlayerForClient(clientId)
            end
        end)

        ECS.subscribe("CLIENT_READY", function(msg)
            local clientId = string.match(msg, "^(%d+)")
            if not clientId then return end
            NetworkSystem.readyClients[clientId] = true
            -- Send initial score snapshot
            local scoreEntities = ECS.getEntitiesWith({"Score"})
            if #scoreEntities > 0 then
                local s = ECS.getComponent(scoreEntities[1], "Score")
                ECS.sendToClient(tonumber(clientId), "GAME_SCORE", tostring(s.value))
            end
            -- Send positions of existing enemies so client renders them right away
            local enemies = ECS.getEntitiesWith({"Enemy", "Transform"})
            for _, id in ipairs(enemies) do
                local t = ECS.getComponent(id, "Transform")
                local phys = ECS.getComponent(id, "Physic")
                local msgOut = formatStateMessage(id, t, phys, 3)
                ECS.sendToClient(tonumber(clientId), "ENTITY_POS", msgOut)
            end
        end)

        ECS.subscribe("RESET_GAME", function(msg)
            print("DEBUG SERVER: Reset Game Requested")
            NetworkSystem.resetWorldState()
        end)

        ECS.subscribe("REQUEST_SPAWN", function(msg)
            local clientId = string.match(msg, "^(%d+)")
            if clientId then
                print("Spawn Request from: " .. clientId)
                if not NetworkSystem.clientEntities[clientId] then
                    NetworkSystem.spawnPlayerForClient(clientId)
                else
                    -- Player already exists, but client is asking (maybe lost packet or restart)
                    print("Resending PLAYER_ASSIGN to " .. clientId)
                    ECS.sendToClient(tonumber(clientId), "PLAYER_ASSIGN", NetworkSystem.clientEntities[clientId])
                end
            end
        end)

        ECS.subscribe("ClientDisconnected", function(msg)
             local clientId = string.match(msg, "^(%d+)")
             if clientId and NetworkSystem.clientEntities[clientId] then
                 ECS.destroyEntity(NetworkSystem.clientEntities[clientId])
                 NetworkSystem.clientEntities[clientId] = nil
                 NetworkSystem.readyClients[clientId] = nil
                 print("Client Disconnected: " .. clientId)

                 -- When the last client leaves, clear the arena for the next joiner.
                 if not hasActiveClients() then
                     NetworkSystem.resetWorldState()
                 end
             end
        end)

        ECS.subscribe("INPUT", function(msg)
            local clientId, key, state = string.match(msg, "(%d+) (%w+) (%d)")
            if clientId and NetworkSystem.clientEntities[clientId] then
                local entityId = NetworkSystem.clientEntities[clientId]
                local input = ECS.getComponent(entityId, "InputState")
                if input then
                    local pressed = (state == "1")
                    if key == "UP" or key == "Z" or key == "W" then input.up = pressed end
                    if key == "DOWN" or key == "S" then input.down = pressed end
                    if key == "LEFT" or key == "Q" or key == "A" then input.left = pressed end
                    if key == "RIGHT" or key == "D" then input.right = pressed end
                    if key == "SPACE" then input.shoot = pressed end
                end
            end
        end)

        ECS.subscribe("ENTITY_DESTROY", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id then
                -- Find which client owned this entity
                for cid, eid in pairs(NetworkSystem.clientEntities) do
                    if eid == id then
                        print("DEBUG SERVER: Player " .. cid .. " Entity Destroyed")
                        NetworkSystem.clientEntities[cid] = nil
                        NetworkSystem.readyClients[cid] = nil
                        break
                    end
                end
            end
        end)

        -- Player death notification from LifeSystem to clean server state and stop broadcasts
        ECS.subscribe("SERVER_PLAYER_DEAD", function(msg)
            local clientId = string.match(msg, "^(%d+)")
            if not clientId then return end
            print("DEBUG SERVER: Player death from LifeSystem for client " .. clientId .. " - resetting world")
            NetworkSystem.clientEntities[clientId] = nil
            NetworkSystem.readyClients[clientId] = nil
            ECS.sendToClient(tonumber(clientId), "CLIENT_RESET", "")
            NetworkSystem.resetWorldState()
        end)

    else
        print("[NetworkSystem] Client Mode")

        ECS.subscribe("PLAYER_ASSIGN", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id then
                print(">> Assigned Player ID: " .. id)
                NetworkSystem.myServerId = id
                NetworkSystem.updateLocalEntity(id, -8, 0, 0, 0, 0, -90, 0, 0, 0, "1")
                -- Send a ready ping; network layer will prefix client id.
                ECS.sendNetworkMessage("CLIENT_READY", "ready")
            end
        end)

        ECS.subscribe("GAME_SCORE", function(msg)
            local scoreVal = tonumber(msg)
            if scoreVal then
                -- Find local score entity and update it
                local scoreEntities = ECS.getEntitiesWith({"Score"})
                if #scoreEntities > 0 then
                    local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
                    scoreComp.value = scoreVal
                end
            end
        end)

        ECS.subscribe("ENTITY_POS", function(msg)
            -- print("DEBUG CLIENT: Received ENTITY_POS " .. msg)
            if not ECS.isGameRunning then return end
            local id, x, y, z, rx, ry, rz, vx, vy, vz, type = string.match(msg, "([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+)")
            if id then
                NetworkSystem.updateLocalEntity(id, x, y, z, rx, ry, rz, vx, vy, vz, type)
            end
        end)

        ECS.subscribe("ENTITY_DESTROY", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id and NetworkSystem.serverEntities[id] then
                ECS.destroyEntity(NetworkSystem.serverEntities[id])
                NetworkSystem.serverEntities[id] = nil
            end
        end)

        ECS.subscribe("ENEMY_DEAD", function(msg)
            if not ECS.isGameRunning then return end
            local id, x, y, z, vx, vy, vz = string.match(msg, "([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+)")
            if not id then return end

            local existing = NetworkSystem.serverEntities[id]
            if existing then
                NetworkSystem.serverEntities[id] = nil
            end

            local nx, ny, nz = tonumber(x) or 0, tonumber(y) or 0, tonumber(z) or 0
            local nvx, nvy, nvz = tonumber(vx) or 0, tonumber(vy) or 0, tonumber(vz) or 0

            local effectEntity = existing or ECS.createEntity()
            if not ECS.getComponent(effectEntity, "Transform") then
                ECS.addComponent(effectEntity, "Transform", Transform(nx, ny, nz))
            else
                local t = ECS.getComponent(effectEntity, "Transform")
                t.x, t.y, t.z = nx, ny, nz
            end

            if not ECS.getComponent(effectEntity, "Mesh") then
                ECS.addComponent(effectEntity, "Mesh", Mesh("assets/models/cube.obj"))
            end
            if not ECS.getComponent(effectEntity, "Color") then
                ECS.addComponent(effectEntity, "Color", Color(1.0, 0.0, 0.0))
            end

            NetworkSystem.deathAnims[id] = {
                entity = effectEntity,
                vx = nvx,
                vy = nvy,
                vz = nvz,
                timer = 0,
                lifetime = 1.2
            }
        end)

        ECS.subscribe("CLIENT_RESET", function(msg)
            print("DEBUG CLIENT: Resetting Network State")
            NetworkSystem.myServerId = nil
            NetworkSystem.serverEntities = {}
            NetworkSystem.deathAnims = {}
            -- Entities are destroyed by MenuSystem cleaning tags,
            -- but clearing the map prevents "ghost" updates.
        end)
    end
end

function NetworkSystem.spawnPlayerForClient(clientId)
    print("DEBUG: Spawning Player for Client " .. clientId)
    local player = ECS.createEntity()
    ECS.addComponent(player, "Transform", Transform(-8, 0, 0, 0, 0, -90))
    ECS.addComponent(player, "Collider", Collider("Box", {1, 1, 1}))
    ECS.addComponent(player, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(player, "Player", Player(20.0))
    ECS.addComponent(player, "Weapon", Weapon(0.2))
    ECS.addComponent(player, "Life", { amount = 3, max = 3, invulnerableTime = 1.0 }) -- 1 second invulnerability
    ECS.addComponent(player, "InputState", { up=false, down=false, left=false, right=false, shoot=false })
    ECS.addComponent(player, "NetworkId", { id = clientId })

    NetworkSystem.clientEntities[clientId] = player
    ECS.sendToClient(tonumber(clientId), "PLAYER_ASSIGN", player)
end

function NetworkSystem.updateLocalEntity(serverId, x, y, z, rx, ry, rz, vx, vy, vz, typeStr)
    local localId = NetworkSystem.serverEntities[serverId]
    local nx, ny, nz = tonumber(x) or 0, tonumber(y) or 0, tonumber(z) or 0
    local nrx, nry, nrz = tonumber(rx) or 0, tonumber(ry) or 0, tonumber(rz) or 0
    local nvx, nvy, nvz = tonumber(vx) or 0, tonumber(vy) or 0, tonumber(vz) or 0
    local nType = tonumber(typeStr) or 1

    if not localId then
        localId = ECS.createEntity()
        ECS.addComponent(localId, "Transform", Transform(nx, ny, nz, nrx, nry, nrz))
        local t = ECS.getComponent(localId, "Transform")
        t.targetX, t.targetY, t.targetZ = nx, ny, nz
        t.targetRX, t.targetRY, t.targetRZ = nrx, nry, nrz
        t.netVX, t.netVY, t.netVZ = nvx, nvy, nvz
        t.netAge = 0

        if nType == 1 then
             ECS.addComponent(localId, "Mesh", Mesh("assets/models/aircraft.obj"))
             ECS.addComponent(localId, "Color", Color(0.0, 1.0, 0.0))
        elseif nType == 2 then
             ECS.addComponent(localId, "Mesh", Mesh("assets/models/cube.obj"))
             ECS.addComponent(localId, "Color", Color(1.0, 1.0, 0.0))
             local t = ECS.getComponent(localId, "Transform")
             t.sx = 0.2; t.sy = 0.2; t.sz = 0.2
        elseif nType == 3 then
             ECS.addComponent(localId, "Mesh", Mesh("assets/models/cube.obj"))
             ECS.addComponent(localId, "Color", Color(1.0, 0.0, 0.0))
             -- local t = ECS.getComponent(localId, "Transform")
             -- t.ry = 90
        end
        NetworkSystem.serverEntities[serverId] = localId
    else
        local t = ECS.getComponent(localId, "Transform")
        if t then
            -- Always drive client-side positions from server, including for my own player (no prediction now).
            t.targetX, t.targetY, t.targetZ = nx, ny, nz
            t.targetRX, t.targetRY, t.targetRZ = nrx, nry, nrz
            t.netVX, t.netVY, t.netVZ = nvx, nvy, nvz
            t.netAge = 0
            if t.x == 0 and t.y == 0 and t.targetX ~= 0 then t.x, t.y, t.z = nx, ny, nz end
        end
    end

    if serverId == NetworkSystem.myServerId and localId then
         local c = ECS.getComponent(localId, "Color")
         if c then c.r = 0.0; c.g = 0.5; c.b = 1.0 end
    end
end

function NetworkSystem.update(dt)
    if not ECS.isServer() then
        local entities = ECS.getEntitiesWith({"Transform"})
        local lerpSpeed = 18.0 -- Slightly softened; prediction handles quickness
        for _, id in ipairs(entities) do
            local t = ECS.getComponent(id, "Transform")
            if t.targetX then
                t.netAge = (t.netAge or 0) + dt
                local age = math.min(t.netAge or 0, 0.2)
                local predictedX = t.targetX + (t.netVX or 0) * age
                local predictedY = t.targetY + (t.netVY or 0) * age
                local predictedZ = t.targetZ + (t.netVZ or 0) * age

                t.x = t.x + (predictedX - t.x) * lerpSpeed * dt
                t.y = t.y + (predictedY - t.y) * lerpSpeed * dt
                t.z = t.z + (predictedZ - t.z) * lerpSpeed * dt
                -- Also interpolate rotation
                t.rx = t.rx + (t.targetRX - t.rx) * lerpSpeed * dt
                t.ry = t.ry + (t.targetRY - t.ry) * lerpSpeed * dt
                t.rz = t.rz + (t.targetRZ - t.rz) * lerpSpeed * dt
            end
        end

        for id, anim in pairs(NetworkSystem.deathAnims) do
            local entity = anim.entity
            local t = entity and ECS.getComponent(entity, "Transform")
            if t then
                anim.vy = anim.vy - 9.8 * dt
                t.x = t.x + (anim.vx or 0) * dt
                t.y = t.y + (anim.vy or 0) * dt
                t.z = t.z + (anim.vz or 0) * dt
                t.ry = t.ry + 180 * dt
            end

            anim.timer = anim.timer + dt
            if anim.timer >= (anim.lifetime or 1.0) then
                if entity then
                    ECS.destroyEntity(entity)
                end
                NetworkSystem.deathAnims[id] = nil
            end
        end
        return
    end

    if not hasReadyClients() then
        return
    end

    NetworkSystem.broadcastTimer = NetworkSystem.broadcastTimer + dt
    if NetworkSystem.broadcastTimer < NetworkSystem.broadcastInterval then return end
    NetworkSystem.broadcastTimer = 0
    NetworkSystem.tickCounter = NetworkSystem.tickCounter + 1

    -- Keep tickCounter bounded to avoid overflow
    if NetworkSystem.tickCounter > 30 then
        NetworkSystem.tickCounter = 0
    end

    NetworkSystem.debugAccum = NetworkSystem.debugAccum + NetworkSystem.broadcastInterval

    local players = ECS.getEntitiesWith({"Player", "Transform"})
    if #players > 0 then
         -- print("DEBUG SERVER: Broadcasting " .. #players .. " Players")
    else
         -- print("DEBUG SERVER: No Players found to broadcast")
    end

    for _, id in ipairs(players) do
        local t = ECS.getComponent(id, "Transform")
        local phys = ECS.getComponent(id, "Physic")
        ECS.broadcastNetworkMessage("ENTITY_POS", formatStateMessage(id, t, phys, 1))
        NetworkSystem.debugSentPlayers = NetworkSystem.debugSentPlayers + 1
    end

    local bullets = ECS.getEntitiesWith({"Bullet", "Transform"})
    if NetworkSystem.tickCounter % 2 == 0 then -- ~every 0.16s
        for _, id in ipairs(bullets) do
            local t = ECS.getComponent(id, "Transform")
            local phys = ECS.getComponent(id, "Physic")
            ECS.broadcastNetworkMessage("ENTITY_POS", formatStateMessage(id, t, phys, 2))
            NetworkSystem.debugSentBullets = NetworkSystem.debugSentBullets + 1
        end
    end

    local enemies = ECS.getEntitiesWith({"Enemy", "Transform"})
    if NetworkSystem.tickCounter % 3 == 0 then -- ~every 0.24s
        for _, id in ipairs(enemies) do
             local t = ECS.getComponent(id, "Transform")
             local phys = ECS.getComponent(id, "Physic")
             local msg = formatStateMessage(id, t, phys, 3)
             ECS.broadcastNetworkMessage("ENTITY_POS", msg)
             NetworkSystem.debugSentEnemies = NetworkSystem.debugSentEnemies + 1
        end
    end

    -- Periodic debug dump (server side) every ~1s
    if NetworkSystem.debugAccum >= 1.0 then
        print(string.format("[NetworkSystem][Server] tick=%d players=%d bullets=%d enemies=%d score=%d readyClients=%d", NetworkSystem.tickCounter, NetworkSystem.debugSentPlayers, NetworkSystem.debugSentBullets, NetworkSystem.debugSentEnemies, NetworkSystem.debugSentScores, countTableKeys(NetworkSystem.readyClients)))
        NetworkSystem.debugAccum = NetworkSystem.debugAccum - 1.0
        NetworkSystem.debugSentPlayers = 0
        NetworkSystem.debugSentBullets = 0
        NetworkSystem.debugSentEnemies = 0
        NetworkSystem.debugSentScores = 0
    end

    -- Broadcast Score
    if NetworkSystem.tickCounter % 10 == 0 then -- score every ~1.2s
        local scoreEntities = ECS.getEntitiesWith({"Score"})
        if #scoreEntities > 0 then
            local s = ECS.getComponent(scoreEntities[1], "Score")
            ECS.broadcastNetworkMessage("GAME_SCORE", tostring(s.value))
            NetworkSystem.debugSentScores = NetworkSystem.debugSentScores + 1
        end
    end
end

ECS.registerSystem(NetworkSystem)
