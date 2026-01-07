local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")
local config = dofile("assets/scripts/space-shooter/config.lua")

-- Expose globally so other systems (MenuSystem) can read connection state.
local NetworkSystem = {}
_G.NetworkSystem = NetworkSystem

NetworkSystem.clientEntities = {}
NetworkSystem.serverEntities = {}
NetworkSystem.myServerId = nil
NetworkSystem.deathAnims = {} -- legacy, no longer used for effects

NetworkSystem.broadcastTimer = 0
NetworkSystem.broadcastInterval = 0.08
NetworkSystem.readyClients = {}
NetworkSystem.tickCounter = 0
NetworkSystem.debugAccum = 0
NetworkSystem.debugSentPlayers = 0
NetworkSystem.debugSentBullets = 0
NetworkSystem.debugSentEnemies = 0
NetworkSystem.debugSentScores = 0

local function destroyEntitySafe(id)
    if id and ECS.getComponent(id, "Transform") then
        ECS.destroyEntity(id)
    end
end

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

local function buildStateData(id, transform, phys, typeNum)
    local vx, vy, vz = extractVelocity(phys)
    return {
        id = id,
        x = transform.x,
        y = transform.y,
        z = transform.z,
        rx = transform.rx,
        ry = transform.ry,
        rz = transform.rz,
        vx = vx,
        vy = vy,
        vz = vz,
        t = typeNum
    }
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
            ECS.broadcastNetworkMessage("ENTITY_DESTROY", id)
            ECS.sendMessage("PhysicCommand", "DestroyBody:" .. id .. ";")
            ECS.destroyEntity(id)
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
    -- ========================================================================
    -- UNIFIED NETWORK SYSTEM: Always subscribe, check caps in callbacks
    -- ========================================================================
    print("[NetworkSystem] Initializing unified subscriptions...")

    -- Create Logic Score Entity (only on authority - we will check later)
    -- local scoreEnt = ECS.createEntity()
    -- ECS.addComponent(scoreEnt, "Score", Score(0))

    ECS.subscribe("ClientConnected", function(msg)
            local clientId = string.match(msg, "^(%d+)")
            if not clientId then return end
            clientId = tonumber(clientId)
            print("Client Connected: " .. clientId)

            -- Check if game is already running
            local gameStateEntities = ECS.getEntitiesWith({"GameState"})
            if #gameStateEntities == 0 then
                print("[NetworkSystem][Server] ERROR: No GameState entity found!")
                return
            end
            local gameState = ECS.getComponent(gameStateEntities[1], "GameState")

            if gameState.state == "PLAYING" then
                print("  -> Spawning player (GameState: PLAYING)")
                if not hasActiveClients() then
                    NetworkSystem.resetWorldState()
                end
                if not NetworkSystem.clientEntities[clientId] then
                    NetworkSystem.spawnPlayerForClient(clientId)
                end
                ECS.isGameRunning = true
            else
                print("  -> Game not started yet (GameState: " .. gameState.state .. ")")

                -- For dedicated server: Wait for explicit start request
                -- Store pending client to spawn later when game starts
                if not NetworkSystem.pendingClients then NetworkSystem.pendingClients = {} end
                NetworkSystem.pendingClients[clientId] = true
            end
        end)

        ECS.subscribe("CLIENT_READY", function(msg)
            local clientId = string.match(msg, "^(%d+)")
            if not clientId then return end
            NetworkSystem.readyClients[clientId] = true
            ECS.isGameRunning = true
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
                local data = buildStateData(id, t, phys, 3)
                if ECS.sendToClientBinary then
                    ECS.sendToClientBinary(tonumber(clientId), "ENTITY_POS", data)
                else
                    -- Fallback impossible if formatStateMessage is gone, but assuming binary is available
                end
            end
        end)

        ECS.subscribe("RESET_GAME", function(msg)
            print("DEBUG SERVER: Reset Game Requested")
            -- Full reset: clear arena and all players/clients to mirror solo restart.
            NetworkSystem.resetWorldState()
            for cid, pid in pairs(NetworkSystem.clientEntities) do
                if pid then
                    ECS.broadcastNetworkMessage("ENTITY_DESTROY", pid)
                    ECS.sendMessage("PhysicCommand", "DestroyBody:" .. pid .. ";")
                    ECS.destroyEntity(pid)
                end
                NetworkSystem.clientEntities[cid] = nil
                NetworkSystem.readyClients[cid] = nil
            end
            NetworkSystem.pendingClients = {}
            ECS.isGameRunning = false
        end)

        -- When game starts (MENU -> PLAYING), spawn pending clients
        ECS.subscribe("GAME_STARTED", function(msg)
            print("[NetworkSystem] Game started")
            
            -- Solo Mode: Spawn local player automatically
            if ECS.capabilities.isLocalMode then
                print("  -> Solo Mode: Spawning local player")
                if not NetworkSystem.clientEntities[0] then
                    NetworkSystem.spawnPlayerForClient(0)
                end
            end

            -- Multi Server: Spawn pending clients who connected before start
            if NetworkSystem.pendingClients then
                for clientId, _ in pairs(NetworkSystem.pendingClients) do
                    print("  -> Spawning pending client " .. clientId)
                    if not hasActiveClients() then
                        NetworkSystem.resetWorldState()
                    end
                    if not NetworkSystem.clientEntities[clientId] then
                        NetworkSystem.spawnPlayerForClient(clientId)
                    end
                end
                NetworkSystem.pendingClients = {}
                ECS.isGameRunning = true
            end
        end)

        ECS.subscribe("REQUEST_GAME_START", function(msg)
            local gameStateEntities = ECS.getEntitiesWith({"GameState"})
            if #gameStateEntities > 0 then
                local gameState = ECS.getComponent(gameStateEntities[1], "GameState")
                if gameState.state == "MENU" then
                    print("  -> Starting game upon client request")
                    ECS.sendMessage("REQUEST_GAME_STATE_CHANGE", "PLAYING")
                    gameState.lastScore = 0
                end
            end
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
             if clientId then
                 clientId = tonumber(clientId)
                 if NetworkSystem.clientEntities[clientId] then
                     ECS.destroyEntity(NetworkSystem.clientEntities[clientId])
                     NetworkSystem.clientEntities[clientId] = nil
                     NetworkSystem.readyClients[clientId] = nil
                     print("Client Disconnected: " .. clientId)

                     -- When the last client leaves, clear the arena for the next joiner.
                     if not hasActiveClients() then
                         NetworkSystem.resetWorldState()
                         ECS.isGameRunning = false
                     end
                 end
             end
        end)

        ECS.subscribe("INPUT", function(msg)
            local clientId, payload = ECS.splitClientIdAndMessage(msg)
            local data = ECS.unpackMsgPack(payload)
            
            local key, state = nil, nil
            if data then
                -- Binary format {k="KEY", s=1/0}
                key = data.k
                state = data.s
            else
                -- Legacy text format fallback: "UP 1"
                -- msg contains "clientId key state" if it wasn't stripped? 
                -- Actually splitClientIdAndMessage returns (id, rest).
                -- So payload is "key state".
                local k, s = string.match(payload, "(%w+) (%d)")
                if k then
                    key = k
                    state = tonumber(s)
                end
            end

            if clientId and key then
                if NetworkSystem.clientEntities[clientId] then
                    local entityId = NetworkSystem.clientEntities[clientId]
                    local input = ECS.getComponent(entityId, "InputState")
                    if input then
                        local pressed = (state == 1 or state == true)
                        if key == "UP" then input.up = pressed end
                        if key == "DOWN" then input.down = pressed end
                        if key == "LEFT" then input.left = pressed end
                        if key == "RIGHT" then input.right = pressed end
                        if key == "SPACE" then input.shoot = pressed end
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
            ECS.isGameRunning = false
        end)

    -- 2. Client-side logic (Always subscribe, logic checks caps)
    print("[NetworkSystem] Initializing client subscriptions...")

        -- Handle sound events broadcast from server
        ECS.subscribe("PLAY_SOUND", function(msg)
            -- msg format: "soundId:path:volume"
            ECS.sendMessage("SoundPlay", msg)
        end)

        -- Handle music stop events from server
        ECS.subscribe("STOP_MUSIC", function(msg)
            ECS.sendMessage("MusicStop", msg)
        end)

        ECS.subscribe("PLAYER_ASSIGN", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id then
                print(">> Assigned Player ID: " .. id)
                NetworkSystem.myServerId = id
                NetworkSystem.updateLocalEntity(id, -8, 0, 0, 0, 0, -90, 0, 0, 0, "1")
                -- Send a ready ping; network layer will prefix client id.
                ECS.sendNetworkMessage("CLIENT_READY", "ready")
                ECS.isGameRunning = true
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
            -- msg can be binary (table) or text (string)
            if not ECS.isGameRunning then return end
            
            local data = ECS.unpackMsgPack(msg)
            if data then
                -- Binary table: {id=..., x=..., ...}
                NetworkSystem.updateLocalEntity(data.id, data.x, data.y, data.z, data.rx, data.ry, data.rz, data.vx, data.vy, data.vz, tostring(data.t))
            else
                -- Legacy text: "id x y z ..."
                local id, x, y, z, rx, ry, rz, vx, vy, vz, type = string.match(msg, "([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+)")
                if id then
                    NetworkSystem.updateLocalEntity(id, x, y, z, rx, ry, rz, vx, vy, vz, type)
                end
            end
        end)

        ECS.subscribe("ENTITY_DESTROY", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id and NetworkSystem.serverEntities[id] then
                destroyEntitySafe(NetworkSystem.serverEntities[id])
                NetworkSystem.serverEntities[id] = nil
            end
        end)

        ECS.subscribe("ENEMY_DEAD", function(msg)
                if not ECS.isGameRunning then return end

                -- Parse: ID X Y Z VX VY VZ
                local id, x, y, z = string.match(msg, "([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+)")

                if id then
                    if x and y and z then
                        Spawns.createExplosion(tonumber(x), tonumber(y), tonumber(z))
                    end

                    local existing = NetworkSystem.serverEntities[id]
                    if existing then
                        destroyEntitySafe(existing)
                        NetworkSystem.serverEntities[id] = nil
                    end
                end
        end)

        ECS.subscribe("CLIENT_RESET", function(msg)
            print("DEBUG CLIENT: Resetting Network State")
            NetworkSystem.myServerId = nil
            NetworkSystem.serverEntities = {}
            ECS.isGameRunning = false
            -- Cleanup any remaining rendered entities to avoid lingering cubes between modes.
            local cleanupIds = ECS.getEntitiesWith({"Transform"})
            for _, eid in ipairs(cleanupIds) do
                -- Preserve UI/menu tags and camera
                local tag = ECS.getComponent(eid, "Tag")
                local camera = ECS.getComponent(eid, "Camera")
                local keep = camera ~= nil
                if tag and not keep then
                    for _, t in ipairs(tag.tags) do
                        if t == "MenuEntity" or t == "GameUI" or t == "GameOverEntity" or t == "ErrorEntity" or t == "LoadingEntity" then
                            keep = true
                            break
                        end
                    end
                end
                if not keep then
                    destroyEntitySafe(eid)
                end
            end
            -- Entities are destroyed by MenuSystem cleaning tags,
            -- but clearing the map prevents "ghost" updates.
        end)
end

function NetworkSystem.spawnPlayerForClient(clientId)
    -- Offset Y based on Client ID to prevent stacking
    local offsetY = (tonumber(clientId) % 4) * 2.0 - 3.0
    local player = Spawns.createPlayer(-8, offsetY, 0, clientId)

    NetworkSystem.clientEntities[clientId] = player
    ECS.sendToClient(tonumber(clientId), "PLAYER_ASSIGN", tostring(player))
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
             -- Try to use numeric ID first, otherwise hash the string
             local pid = tonumber(serverId)
             if not pid then
                 local s = tostring(serverId)
                 pid = 0
                 for i = 1, #s do pid = pid + string.byte(s, i) end
             end

             -- Unique Mesh per Player
             local meshes = {
                "assets/models/aircraft.obj",
                "assets/models/delta_wing.obj",
                "assets/models/fighter.obj",
                "assets/models/bomber.obj"
             }
             -- Use a different modulus for mesh to mix it up
             -- Add a prime number offset to desync mesh and color cycles
             local meshIndex = ((pid * 3) % #meshes) + 1
             ECS.addComponent(localId, "Mesh", Mesh(meshes[meshIndex]))
             -- Unique Color per Player
             local colors = {
                {1.0, 0.0, 0.0}, -- Red
                {0.0, 1.0, 0.0}, -- Green
                {0.0, 0.0, 1.0}, -- Blue
                {1.0, 1.0, 0.0}, -- Yellow
                {1.0, 0.0, 1.0}, -- Magenta
                {0.0, 1.0, 1.0}, -- Cyan
                {1.0, 0.5, 0.0}, -- Orange
                {0.5, 0.0, 1.0}, -- Purple
             }
             local colorIndex = ((pid * 7) % #colors) + 1
             local col = colors[colorIndex]

             ECS.addComponent(localId, "Color", Color(col[1], col[2], col[3]))

             -- Reactor Particles (Blue Trail) for ALL players (Local and Remote)
             ECS.addComponent(localId, "ParticleGenerator", ParticleGenerator(
                -1.0, 0, 0,   -- Offset (Behind)
                -1, 0, 0,     -- Direction (Backwards)
                0.2,          -- Spread
                2.0,          -- Speed
                0.5,          -- LifeTime
                50.0,         -- Rate
                0.1,          -- Size
                0.0, 0.5, 1.0 -- Color (Blue)
             ))

             -- If this is ME, add prediction components so InputSystem can drive it locally
             if serverId == NetworkSystem.myServerId then
                 ECS.addComponent(localId, "InputState", { up=false, down=false, left=false, right=false, shoot=false })
                 ECS.addComponent(localId, "Player", Player(config.player.speed))
                 ECS.addComponent(localId, "Weapon", Weapon(config.player.weaponCooldown))
                 ECS.addComponent(localId, "Physic", Physic(1.0, 0.0, false, false))
                 -- We do NOT add Collider yet, to avoid local collision resolution conflicts.
                 -- We trust the server for collisions.
             end

        elseif nType == 2 then
             ECS.addComponent(localId, "Mesh", Mesh("assets/models/cube.obj"))
             ECS.addComponent(localId, "Color", Color(1.0, 1.0, 0.0))
             local t = ECS.getComponent(localId, "Transform")
             t.sx = 0.2; t.sy = 0.2; t.sz = 0.2
        elseif nType == 3 then
             ECS.addComponent(localId, "Mesh", Mesh("assets/models/cube.obj"))
             ECS.addComponent(localId, "Color", Color(1.0, 0.0, 0.0))
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

    -- ENSURE LOCAL PREDICTION COMPONENTS ARE PRESENT
    -- This handles the case where ENTITY_POS arrives before PLAYER_ASSIGN.
    if serverId == NetworkSystem.myServerId and localId then
         -- local c = ECS.getComponent(localId, "Color")
         -- if c then c.r = 0.0; c.g = 0.5; c.b = 1.0 end

         if not ECS.hasComponent(localId, "InputState") then
             ECS.addComponent(localId, "InputState", { up=false, down=false, left=false, right=false, shoot=false })
             ECS.addComponent(localId, "Player", Player(config.player.speed))
             ECS.addComponent(localId, "Weapon", Weapon(config.player.weaponCooldown))
             ECS.addComponent(localId, "Physic", Physic(1.0, 0.0, true, false))
         end
    end
end

function NetworkSystem.update(dt)
    -- ========================================================================
    -- CLIENT-SIDE: Interpolation Logic
    -- ========================================================================
    if not ECS.capabilities.hasAuthority and ECS.capabilities.hasNetworkSync then
        if not ECS.isGameRunning then
            return
        end
        local entities = ECS.getEntitiesWith({"Transform"})
        local lerpSpeed = 18.0 -- Slightly softened; prediction handles quickness
        for _, id in ipairs(entities) do
            -- Skip interpolation for our own player (Client-Side Prediction)
            local myEntityId = NetworkSystem.myServerId and NetworkSystem.serverEntities[NetworkSystem.myServerId]
            local isMyPlayer = (myEntityId == id)

            if not isMyPlayer then
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
            else
                -- Reconciliation for Local Player
                -- If local prediction diverges too much from server authority, pull it back.
                local t = ECS.getComponent(id, "Transform")
                if t and t.targetX then
                    local dx = t.x - t.targetX
                    local dy = t.y - t.targetY
                    local distSq = dx*dx + dy*dy
                    -- Threshold: 2.0 units (approx 200ms lag at speed 10)
                    -- If we are farther than that, something is wrong (packet loss/drift).
                    if distSq > 4.0 then
                        local correctionSpeed = 5.0 * dt
                        t.x = t.x + (t.targetX - t.x) * correctionSpeed
                        t.y = t.y + (t.targetY - t.y) * correctionSpeed
                    end
                end
            end
        end

        return
    end

    -- ========================================================================
    -- SERVER-SIDE: State Broadcasting Logic
    -- ========================================================================
    if not ECS.capabilities.hasAuthority or not ECS.capabilities.hasNetworkSync then
        return
    end

    if not hasReadyClients() or not ECS.isGameRunning then
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
        if ECS.broadcastBinary then
            ECS.broadcastBinary("ENTITY_POS", buildStateData(id, t, phys, 1))
        else
            -- Should not happen with new engine
        end
        NetworkSystem.debugSentPlayers = NetworkSystem.debugSentPlayers + 1
    end

    local bullets = ECS.getEntitiesWith({"Bullet", "Transform"})
    if NetworkSystem.tickCounter % 2 == 0 then -- ~every 0.16s
        for _, id in ipairs(bullets) do
            local t = ECS.getComponent(id, "Transform")
            local phys = ECS.getComponent(id, "Physic")
            if ECS.broadcastBinary then
                ECS.broadcastBinary("ENTITY_POS", buildStateData(id, t, phys, 2))
            end
            NetworkSystem.debugSentBullets = NetworkSystem.debugSentBullets + 1
        end
    end

    local enemies = ECS.getEntitiesWith({"Enemy", "Transform"})
    if NetworkSystem.tickCounter % 3 == 0 then -- ~every 0.24s
        for _, id in ipairs(enemies) do
             local t = ECS.getComponent(id, "Transform")
             local phys = ECS.getComponent(id, "Physic")
             if ECS.broadcastBinary then
                 ECS.broadcastBinary("ENTITY_POS", buildStateData(id, t, phys, 3))
             end
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
