local NetworkSystem = {}

NetworkSystem.clientEntities = {} 
NetworkSystem.serverEntities = {}
NetworkSystem.myServerId = nil

NetworkSystem.broadcastTimer = 0
NetworkSystem.broadcastInterval = 0.05

function NetworkSystem.init()
    if ECS.isServer() then
        print("[NetworkSystem] Server Mode")

        ECS.subscribe("ClientConnected", function(msg)
            local clientId = string.match(msg, "^(%d+)")
            print("Client Connected: " .. clientId .. " (Auto-Spawning)")
            if not NetworkSystem.clientEntities[clientId] then
                NetworkSystem.spawnPlayerForClient(clientId)
            end
        end)

        ECS.subscribe("REQUEST_SPAWN", function(msg)
            local clientId = string.match(msg, "^(%d+)")
            if clientId then
                print("Spawn Request from: " .. clientId)
                if not NetworkSystem.clientEntities[clientId] then
                    NetworkSystem.spawnPlayerForClient(clientId)
                end
            end
        end)

        ECS.subscribe("ClientDisconnected", function(msg)
             local clientId = string.match(msg, "^(%d+)")
             if clientId and NetworkSystem.clientEntities[clientId] then
                 ECS.destroyEntity(NetworkSystem.clientEntities[clientId])
                 NetworkSystem.clientEntities[clientId] = nil
                 print("Client Disconnected: " .. clientId)
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

    else
        print("[NetworkSystem] Client Mode")

        ECS.subscribe("PLAYER_ASSIGN", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id then
                print(">> Assigned Player ID: " .. id)
                NetworkSystem.myServerId = id
                NetworkSystem.updateLocalEntity(id, -8, 0, 0, 0, 0, -90, "1")
            end
        end)
        
        ECS.subscribe("ENTITY_POS", function(msg)
            -- print("DEBUG CLIENT: Received ENTITY_POS " .. msg)
            local id, x, y, z, rx, ry, rz, type = string.match(msg, "([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+)")
            if id then
                NetworkSystem.updateLocalEntity(id, x, y, z, rx, ry, rz, type)
            end
        end)

        ECS.subscribe("ENTITY_DESTROY", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id and NetworkSystem.serverEntities[id] then
                ECS.destroyEntity(NetworkSystem.serverEntities[id])
                NetworkSystem.serverEntities[id] = nil
            end
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
    ECS.addComponent(player, "Life", Life(3))
    ECS.addComponent(player, "InputState", { up=false, down=false, left=false, right=false, shoot=false })
    ECS.addComponent(player, "NetworkId", { id = clientId }) 

    NetworkSystem.clientEntities[clientId] = player
    ECS.sendToClient(tonumber(clientId), "PLAYER_ASSIGN", player)
end

function NetworkSystem.updateLocalEntity(serverId, x, y, z, rx, ry, rz, typeStr)
    local localId = NetworkSystem.serverEntities[serverId]
    local nx, ny, nz = tonumber(x) or 0, tonumber(y) or 0, tonumber(z) or 0
    local nrx, nry, nrz = tonumber(rx) or 0, tonumber(ry) or 0, tonumber(rz) or 0
    local nType = tonumber(typeStr) or 1

    if not localId then
        localId = ECS.createEntity()
        ECS.addComponent(localId, "Transform", Transform(nx, ny, nz, nrx, nry, nrz))
        
        if nType == 1 then
             ECS.addComponent(localId, "Mesh", Mesh("assets/models/aircraft.obj"))
             ECS.addComponent(localId, "Color", Color(0.0, 1.0, 0.0))
        elseif nType == 2 then
             ECS.addComponent(localId, "Mesh", Mesh("assets/models/cube.obj"))
             ECS.addComponent(localId, "Color", Color(1.0, 1.0, 0.0))
             local t = ECS.getComponent(localId, "Transform")
             t.sx = 0.2; t.sy = 0.2; t.sz = 0.2
        elseif nType == 3 then
             ECS.addComponent(localId, "Mesh", Mesh("assets/models/aircraft.obj"))
             ECS.addComponent(localId, "Color", Color(1.0, 0.0, 0.0))
             local t = ECS.getComponent(localId, "Transform")
             t.ry = 90
        end
        NetworkSystem.serverEntities[serverId] = localId
    else
        local t = ECS.getComponent(localId, "Transform")
        if t then
            t.targetX, t.targetY, t.targetZ = nx, ny, nz
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
        local lerpSpeed = 10.0
        for _, id in ipairs(entities) do
            local t = ECS.getComponent(id, "Transform")
            if t.targetX then
                t.x = t.x + (t.targetX - t.x) * lerpSpeed * dt
                t.y = t.y + (t.targetY - t.y) * lerpSpeed * dt
                t.z = t.z + (t.targetZ - t.z) * lerpSpeed * dt
            end
        end
        return
    end

    NetworkSystem.broadcastTimer = NetworkSystem.broadcastTimer + dt
    if NetworkSystem.broadcastTimer < NetworkSystem.broadcastInterval then return end
    NetworkSystem.broadcastTimer = 0

    local players = ECS.getEntitiesWith({"Player", "Transform"})
    if #players > 0 then
         -- print("DEBUG SERVER: Broadcasting " .. #players .. " Players") 
    else
         -- print("DEBUG SERVER: No Players found to broadcast")
    end

    for _, id in ipairs(players) do
        local t = ECS.getComponent(id, "Transform")
        ECS.broadcastNetworkMessage("ENTITY_POS", string.format("%s %f %f %f %f %f %f 1", id, t.x, t.y, t.z, t.rx, t.ry, t.rz))
    end
    
    local bullets = ECS.getEntitiesWith({"Bullet", "Transform"})
    for _, id in ipairs(bullets) do
        local t = ECS.getComponent(id, "Transform")
        ECS.broadcastNetworkMessage("ENTITY_POS", string.format("%s %f %f %f %f %f %f 2", id, t.x, t.y, t.z, t.rx, t.ry, t.rz))
    end

    local enemies = ECS.getEntitiesWith({"Enemy", "Transform"})
    for _, id in ipairs(enemies) do
         local t = ECS.getComponent(id, "Transform")
         ECS.broadcastNetworkMessage("ENTITY_POS", string.format("%s %f %f %f %f %f %f 3", id, t.x, t.y, t.z, t.rx, t.ry, t.rz))
    end
end

ECS.registerSystem(NetworkSystem)
