-- Network System for platformer host/client synchronization

local NetworkSystem = {}

local ADVANCED_PLAYER_MODEL = "assets/models/Monster_3/motion_1.obj"

NetworkSystem.tick = 0
NetworkSystem.stateSendInterval = 2
NetworkSystem.clientInputs = {}
NetworkSystem.serverToLocal = {}
NetworkSystem.clientToPlayer = {}
NetworkSystem.lastJoinTime = {}
NetworkSystem.hostPlayerId = nil
NetworkSystem.assignedServerPlayerId = nil

local function ensureInputTable(clientId)
    if not NetworkSystem.clientInputs[clientId] then
        NetworkSystem.clientInputs[clientId] = {
            forward = false,
            backward = false,
            left = false,
            right = false,
            rotateLeft = false,
            rotateRight = false,
            jump = false,
            jumpRequested = false
        }
    end
    return NetworkSystem.clientInputs[clientId]
end

local function applyInputKey(input, key, pressed)
    if key == "Z" or key == "W" then
        input.forward = pressed
    elseif key == "S" then
        input.backward = pressed
    elseif key == "Q" or key == "A" then
        input.left = pressed
    elseif key == "D" then
        input.right = pressed
    elseif key == "LEFT" then
        input.rotateLeft = pressed
    elseif key == "RIGHT" then
        input.rotateRight = pressed
    elseif key == "SPACE" then
        input.jump = pressed
        if pressed then
            input.jumpRequested = true
        end
    end
end

local function findOrCreateLocalEntity(serverId)
    local mapped = NetworkSystem.serverToLocal[serverId]
    if mapped then
        return mapped
    end

    if NetworkSystem.assignedServerPlayerId == serverId then
        local players = ECS.getEntitiesWith({"Player", "Transform"})
        if #players > 0 then
            NetworkSystem.serverToLocal[serverId] = players[1]
            return players[1]
        end
    end

    local e = ECS.createEntity()
    ECS.addComponent(e, "Transform", Transform(0, 3, 0))
    ECS.addComponent(e, "Mesh", Mesh(ADVANCED_PLAYER_MODEL))
    ECS.addComponent(e, "Color", Color(0.95, 0.55, 0.2))
    ECS.addComponent(e, "Player", Player(12.0, 100.0))
    ECS.addComponent(e, "Physic", Physic(0.0, 0.0, true))
    NetworkSystem.serverToLocal[serverId] = e
    return e
end

local function spawnControlledPlayerForClient(clientId)
    -- Reuse an already spawned remote-controlled player if it exists.
    local existing = ECS.getEntitiesWith({"RemoteControl", "Player", "Transform"})
    for _, eid in ipairs(existing) do
        local rc = ECS.getComponent(eid, "RemoteControl")
        if rc and tonumber(rc.clientId) == tonumber(clientId) then
            NetworkSystem.clientToPlayer[clientId] = eid
            return eid
        end
    end

    local spawn = GameLogicSystem and GameLogicSystem.lastCheckpoint or {x = 0, y = 3, z = 0}
    local e = ECS.createEntity()
    ECS.addComponent(e, "Transform", Transform(spawn.x + (clientId % 2) * 2, spawn.y, spawn.z + 2))
    ECS.addComponent(e, "Mesh", Mesh(ADVANCED_PLAYER_MODEL))
    ECS.addComponent(e, "Collider", Collider("Box", {1, 1, 1}))
    ECS.addComponent(e, "Physic", Physic(15.0, 4.0, true))
    ECS.addComponent(e, "Player", Player(12.0, 100.0))
    ECS.addComponent(e, "Color", Color(0.95, 0.55, 0.2))
    ECS.addComponent(e, "RemoteControl", { clientId = clientId })

    NetworkSystem.clientToPlayer[clientId] = e
    NetworkSystem.serverToLocal[tostring(e)] = e
    return e
end

local function removeEntityFromLocalMaps(entityId)
    for k, v in pairs(NetworkSystem.serverToLocal) do
        if v == entityId then
            NetworkSystem.serverToLocal[k] = nil
        end
    end
end

local function destroyRemotePlayerForClient(clientId)
    local eid = NetworkSystem.clientToPlayer[clientId]
    if not eid then
        return
    end
    removeEntityFromLocalMaps(eid)
    NetworkSystem.clientToPlayer[clientId] = nil
    NetworkSystem.clientInputs[clientId] = nil
    ECS.destroyEntity(eid)
end

local function enforceSingleRemotePlayerPerClient()
    local remotePlayers = ECS.getEntitiesWith({"RemoteControl", "Player", "Transform"})
    local firstByClient = {}
    local duplicates = {}

    for _, eid in ipairs(remotePlayers) do
        local rc = ECS.getComponent(eid, "RemoteControl")
        local cid = rc and tonumber(rc.clientId) or nil
        if cid then
            if not firstByClient[cid] then
                firstByClient[cid] = eid
            else
                table.insert(duplicates, eid)
            end
        end
    end

    for cid, eid in pairs(NetworkSystem.clientToPlayer) do
        local rc = ECS.getComponent(eid, "RemoteControl")
        if not rc or tonumber(rc.clientId) ~= tonumber(cid) then
            NetworkSystem.clientToPlayer[cid] = nil
        end
    end

    for cid, eid in pairs(firstByClient) do
        NetworkSystem.clientToPlayer[cid] = eid
    end

    for _, eid in ipairs(duplicates) do
        local rc = ECS.getComponent(eid, "RemoteControl")
        local cid = rc and tonumber(rc.clientId) or -1
        print("[PlatformerNetwork] Removing duplicate player " .. tostring(eid) .. " for client " .. tostring(cid))
        removeEntityFromLocalMaps(eid)
        ECS.destroyEntity(eid)
    end
end

function NetworkSystem.init()
    print("[PlatformerNetwork] Initialized")

    if not ECS.capabilities.hasAuthority then
        ECS.sendNetworkMessage("PLAYER_JOIN", "join")
    end

    ECS.subscribe("PLAYER_JOIN", function(msg)
        if not ECS.capabilities.hasAuthority then return end
        enforceSingleRemotePlayerPerClient()

        local clientId = nil
        local parsedId, _ = ECS.splitClientIdAndMessage(msg)
        if parsedId and parsedId > 0 then
            clientId = parsedId
        else
            clientId = string.match(msg, "^(%d+)")
        end
        if not clientId then return end
        clientId = tonumber(clientId)

        local now = os.clock()
        local last = NetworkSystem.lastJoinTime[clientId] or 0
        if now - last < 0.5 then
            local alreadyAssigned = NetworkSystem.clientToPlayer[clientId]
            if alreadyAssigned then
                ECS.sendToClient(clientId, "PLAYER_ASSIGN", tostring(alreadyAssigned))
            end
            return
        end
        NetworkSystem.lastJoinTime[clientId] = now

        if not NetworkSystem.hostPlayerId then
            local players = ECS.getEntitiesWith({"Player", "Transform"})
            if #players > 0 then
                NetworkSystem.hostPlayerId = players[1]
            end
        end

        local remotePlayerId = NetworkSystem.clientToPlayer[clientId]
        if not remotePlayerId then
            remotePlayerId = spawnControlledPlayerForClient(clientId)
        else
            local stillThere = ECS.getComponent(remotePlayerId, "Player")
            if not stillThere then
                remotePlayerId = spawnControlledPlayerForClient(clientId)
            end
        end

        ECS.sendToClient(clientId, "PLAYER_ASSIGN", tostring(remotePlayerId))
        print("[PlatformerNetwork] Assigned player " .. remotePlayerId .. " to client " .. clientId)
    end)

    ECS.subscribe("ClientDisconnected", function(msg)
        if not ECS.capabilities.hasAuthority then return end
        local clientId = string.match(msg, "^(%d+)")
        if not clientId then return end
        clientId = tonumber(clientId)
        print("[PlatformerNetwork] Client disconnected: " .. clientId)
        destroyRemotePlayerForClient(clientId)
    end)

    ECS.subscribe("PLAYER_ASSIGN", function(msg)
        if ECS.capabilities.hasAuthority then return end
        NetworkSystem.assignedServerPlayerId = tostring(msg)
        print("[PlatformerNetwork] Client assigned player id: " .. NetworkSystem.assignedServerPlayerId)
    end)

    ECS.subscribe("PINPUT", function(msg)
        if not ECS.capabilities.hasAuthority then return end
        local clientId, payload = ECS.splitClientIdAndMessage(msg)
        if not clientId or not payload then
            return
        end

        local key, stateStr = string.match(payload, "([^%s]+)%s+([^%s]+)")
        local state = tonumber(stateStr)
        if not clientId or not key or state == nil then
            return
        end

        local input = ensureInputTable(clientId)
        applyInputKey(input, key, state == 1)
    end)

    ECS.subscribe("PSTATE", function(msg)
        if ECS.capabilities.hasAuthority then return end

        local id, x, y, z, rx, ry, rz, vx, vy, vz =
            string.match(msg, "([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+)")

        if not id then return end
        local localEntity = findOrCreateLocalEntity(id)
        local t = ECS.getComponent(localEntity, "Transform")
        if t then
            t.x = tonumber(x) or t.x
            t.y = tonumber(y) or t.y
            t.z = tonumber(z) or t.z
            t.rx = tonumber(rx) or t.rx
            t.ry = tonumber(ry) or t.ry
            t.rz = tonumber(rz) or t.rz
        end

        local p = ECS.getComponent(localEntity, "Physic")
        if p then
            p.vx = tonumber(vx) or 0
            p.vy = tonumber(vy) or 0
            p.vz = tonumber(vz) or 0
        end
    end)

    _G.PlatformerNetwork = NetworkSystem
end

function NetworkSystem.update(dt)
    if not ECS.capabilities.hasAuthority then
        return
    end

    -- Safety invariant: exactly one remote-controlled player per client.
    enforceSingleRemotePlayerPerClient()

    for clientId, entityId in pairs(NetworkSystem.clientToPlayer) do
        local input = ensureInputTable(clientId)
        local transform = ECS.getComponent(entityId, "Transform")
        local physic = ECS.getComponent(entityId, "Physic")
        local playerComp = ECS.getComponent(entityId, "Player")

        if transform and physic and playerComp then
            local rotationSpeed = 3.0
            local vaz = 0
            if input.rotateLeft then vaz = vaz + rotationSpeed end
            if input.rotateRight then vaz = vaz - rotationSpeed end
            physic.vaz = vaz

            local speed = playerComp.speed or 12.0
            local ry = transform.ry or 0
            local forwardX = math.sin(ry)
            local forwardZ = math.cos(ry)
            local rightX = math.cos(ry)
            local rightZ = -math.sin(ry)

            local vx = 0
            local vz = 0
            if input.forward then vx = vx + forwardX; vz = vz + forwardZ end
            if input.backward then vx = vx - forwardX; vz = vz - forwardZ end
            if input.left then vx = vx + rightX; vz = vz + rightZ end
            if input.right then vx = vx - rightX; vz = vz - rightZ end

            local length = math.sqrt(vx * vx + vz * vz)
            if length > 0 then
                vx = (vx / length) * speed
                vz = (vz / length) * speed
            end

            physic.vx = vx
            physic.vz = vz

            if playerComp.jumpCooldown and playerComp.jumpCooldown > 0 then
                playerComp.jumpCooldown = playerComp.jumpCooldown - dt
            end

            if input.jumpRequested and playerComp.isGrounded and (playerComp.jumpCooldown or 0) <= 0 then
                ECS.sendMessage("PhysicCommand", "ApplyImpulse:" .. entityId .. ":0," .. playerComp.jumpForce .. ",0;")
                playerComp.isGrounded = false
                playerComp.jumpCooldown = playerComp.jumpCooldownTime or 0.5
                input.jumpRequested = false
            end
        end
    end

    NetworkSystem.tick = NetworkSystem.tick + 1
    if NetworkSystem.tick % NetworkSystem.stateSendInterval ~= 0 then
        return
    end

    local players = ECS.getEntitiesWith({"Player", "Transform", "Physic"})
    for _, id in ipairs(players) do
        local t = ECS.getComponent(id, "Transform")
        local p = ECS.getComponent(id, "Physic")
        if t and p then
            local payload = string.format(
                "%s %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f",
                id, t.x, t.y, t.z, t.rx, t.ry, t.rz, p.vx or 0, p.vy or 0, p.vz or 0
            )
            ECS.broadcastNetworkMessage("PSTATE", payload)
        end
    end
end

ECS.registerSystem(NetworkSystem)
return NetworkSystem
