local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")
local config = dofile("assets/scripts/space-shooter/config.lua")

local NetworkSystem = {}
_G.NetworkSystem = NetworkSystem

NetworkSystem.clientEntities = {} -- clientId -> entityId
NetworkSystem.serverEntities = {} -- remoteId -> localProxyId
NetworkSystem.myServerId = nil    -- My actual entityId (local or remote)

NetworkSystem.broadcastTimer = 0
NetworkSystem.broadcastInterval = 0.05 -- 20Hz

local function isPlaying()
    local gsEntities = ECS.getEntitiesWith({"GameState"})
    if #gsEntities > 0 then
        local gs = ECS.getComponent(gsEntities[1], "GameState")
        return gs.state == "PLAYING"
    end
    return false
end

function NetworkSystem.init()
    print("[NetworkSystem] Initializing Robust Parity System...")

    ECS.subscribe("INPUT", function(msg)
        if not (ECS.capabilities.hasAuthority and isPlaying()) then return end
        local clientId, payload = ECS.splitClientIdAndMessage(msg)
        local data = ECS.unpackMsgPack(payload)

        if data and NetworkSystem.clientEntities[clientId] then
            local e = NetworkSystem.clientEntities[clientId]
            local input = ECS.getComponent(e, "InputState")
            if input then
                local pressed = (data.s == 1)
                if data.k == "UP" then input.up = pressed end
                if data.k == "DOWN" then input.down = pressed end
                if data.k == "LEFT" then input.left = pressed end
                if data.k == "RIGHT" then input.right = pressed end
                if data.k == "SPACE" then input.shoot = pressed end
            end
        end
    end)

    ECS.subscribe("ENTITY_POS", function(msg)
        if not (ECS.capabilities.hasRendering and isPlaying()) then return end
        local data = ECS.unpackMsgPack(msg)
        if not data then return end

        if ECS.hasComponent(data.id, "Transform") and not NetworkSystem.serverEntities[data.id] then
            return
        end

        NetworkSystem.updateRemoteEntity(data)
    end)

    ECS.subscribe("PLAYER_ASSIGN", function(msg)
        if not ECS.capabilities.hasRendering then return end
        print("[NetworkSystem] PLAYER_ASSIGN: My ID is now " .. msg)
        NetworkSystem.myServerId = msg
        ECS.isGameRunning = true
    end)

    ECS.subscribe("GAME_STARTED", function()
        if ECS.capabilities.isLocalMode then
            print("[NetworkSystem] Solo Mode: Spawning Player 0")
            NetworkSystem.spawnPlayerForClient(0)
        end
    end)
end

function NetworkSystem.spawnPlayerForClient(clientId)
    local player = Spawns.createPlayer(-8, (clientId % 4) * 2 - 2, 0, clientId)
    NetworkSystem.clientEntities[clientId] = player
    ECS.broadcastNetworkMessage("PLAYER_ASSIGN", player)
end

function NetworkSystem.updateRemoteEntity(data)
    local localId = NetworkSystem.serverEntities[data.id]
    if not localId then
        if ECS.hasComponent(data.id, "Transform") then return end

        print("[NetworkSystem] New remote entity: " .. data.id)
        localId = ECS.createEntity()
        ECS.addComponent(localId, "Transform", Transform(data.x, data.y, data.z))
        if data.t == 1 then
            ECS.addComponent(localId, "Mesh", Mesh("assets/models/aircraft.obj"))
            ECS.addComponent(localId, "Color", Color(0, 1, 1))
        elseif data.t == 2 then
            ECS.addComponent(localId, "Mesh", Mesh("assets/models/cube.obj"))
            local t = ECS.getComponent(localId, "Transform")
            t.sx, t.sy, t.sz = 0.2, 0.2, 0.2
        end
        NetworkSystem.serverEntities[data.id] = localId
    else
        local t = ECS.getComponent(localId, "Transform")
        if t then
            t.x, t.y, t.z = data.x, data.y, data.z
        end
    end
end

function NetworkSystem.update(dt)
    if not (ECS.capabilities.hasAuthority and isPlaying()) then return end

    NetworkSystem.broadcastTimer = NetworkSystem.broadcastTimer + dt
    if NetworkSystem.broadcastTimer < NetworkSystem.broadcastInterval then return end
    NetworkSystem.broadcastTimer = 0

    local entities = ECS.getEntitiesWith({"Transform"})
    for _, id in ipairs(entities) do
        local t = ECS.getComponent(id, "Transform")
        local typeNum = 0
        if ECS.hasComponent(id, "Player") then typeNum = 1
        elseif ECS.hasComponent(id, "Bullet") then typeNum = 2
        elseif ECS.hasComponent(id, "Enemy") then typeNum = 3 end

        if typeNum > 0 then
            ECS.broadcastBinary("ENTITY_POS", { id=id, x=t.x, y=t.y, z=t.z, t=typeNum })
        end
    end
end

ECS.registerSystem(NetworkSystem)
return NetworkSystem