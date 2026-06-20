-- ============================================================================
-- NetworkSystem.lua - Clean Game State System
-- ============================================================================
-- Server-side: Entity spawning, position broadcasting
-- Client-side: Entity interpolation, heartbeat, state handling
-- Game state transitions handled by C++ GameStateMachine
-- ============================================================================

local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")
local config = dofile("assets/scripts/space-shooter/config.lua")
local ScoreSystem = _G.ScoreSystem or require("assets/scripts/space-shooter/systems/ScoreSystem")

-- Global exposure for other systems
local NetworkSystem = {}
_G.NetworkSystem = NetworkSystem

-- ============================================================================
-- STATE TRACKING
-- ============================================================================
NetworkSystem.clientEntities = {}      -- Server: clientId -> entityId
NetworkSystem.serverEntities = {}      -- Client: serverId -> localEntityId
NetworkSystem.myServerId = nil         -- Client: my assigned entity ID
NetworkSystem.clientDeathReported = false

-- Client state (matches C++ enum)
NetworkSystem.CLIENT_STATE = {
    MENU = "MENU",
    LOBBY = "LOBBY",
    IN_GAME = "IN_GAME"
}
NetworkSystem.clientState = NetworkSystem.CLIENT_STATE.MENU

-- Server tracking (delegates to C++ GameStateMachine for state)
NetworkSystem.playerSessions = {}      -- Server: clientId -> session data
NetworkSystem.readyClients = {}        -- Server: set of ready client IDs
NetworkSystem.joinedClients = {}       -- Server: set of joined client IDs

-- Broadcasting
NetworkSystem.broadcastTimer = 0
NetworkSystem.broadcastInterval = 0.10
NetworkSystem.tickCounter = 0

-- Client heartbeat
NetworkSystem.heartbeatTimer = 0
NetworkSystem.heartbeatInterval = 1.0  -- Send heartbeat every 1 second

-- ============================================================================
-- UTILITY FUNCTIONS
-- ============================================================================
local function destroyEntitySafe(id)
    if id and ECS.getComponent(id, "Transform") then
        ECS.sendMessage("PhysicCommand", "DestroyBody:" .. tostring(id) .. ";")
        ECS.destroyEntity(id)
    end
end

local function nowSeconds()
    return os.clock()
end

local function extractVelocity(phys)
    if not phys then return 0, 0, 0 end
    return phys.vx or 0, phys.vy or 0, phys.vz or 0
end

local function buildStateData(id, transform, phys, typeNum)
    local enemyTypeComp = ECS.getComponent(id, "EnemyType")
    local actualType = enemyTypeComp and enemyTypeComp.type or typeNum
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
        t = actualType
    }
end

local function getScaleForType(typeNum)
    if typeNum == 1 then
        return config.player.scale, config.player.scale, config.player.scale
    end
    if typeNum == 2 or typeNum == 3 then
        return 0.2, 0.2, 0.2
    end
    if typeNum == 99 then
        return 6.2, 6.2, 6.2
    end
    if typeNum == 61 or typeNum == 62 or typeNum == 63 then
        return 0.7, 0.7, 0.7
    end
    return config.enemy.scale, config.enemy.scale, config.enemy.scale
end

local function resolveCurrentLevel()
    if _G.CurrentLevel then
        return tonumber(_G.CurrentLevel) or 1
    end
    local file = io.open("current_level.txt", "r")
    if file then
        local content = file:read("*all")
        file:close()
        return tonumber(content) or 1
    end
    return 1
end

local function resetLevelToOne()
    _G.CurrentLevel = 1
    local file = io.open("current_level.txt", "w")
    if file then
        file:write("1")
        file:close()
    end
end

-- ============================================================================
-- PLAYER SESSION HELPERS (Server-side, lightweight - C++ handles state)
-- ============================================================================
local function ensurePlayerSession(clientId)
    if not clientId then return nil end
    if not NetworkSystem.playerSessions[clientId] then
        NetworkSystem.playerSessions[clientId] = {
            playerId = clientId,
            entityId = nil,
            lastPacketAt = nowSeconds()
        }
    end
    return NetworkSystem.playerSessions[clientId]
end

local function touchPlayerSession(clientId)
    local session = ensurePlayerSession(clientId)
    if session then
        session.lastPacketAt = nowSeconds()
    end
    return session
end

-- ============================================================================
-- ENTITY CLEANUP (Server-side)
-- ============================================================================
local function cleanupPlayerEntity(clientId)
    local session = ensurePlayerSession(clientId)
    local playerId = NetworkSystem.clientEntities[clientId]
    if not playerId and session then
        playerId = session.entityId
    end

    if playerId then
        ECS.broadcastNetworkMessage("ENTITY_DESTROY", tostring(playerId))
        destroyEntitySafe(playerId)
    end

    NetworkSystem.clientEntities[clientId] = nil
    if session then
        session.entityId = nil
    end

    return playerId
end

-- ============================================================================
-- WORLD STATE RESET (Server-side)
-- ============================================================================
function NetworkSystem.resetWorldState()
    local function destroyAndBroadcast(entities)
        for _, id in ipairs(entities) do
            ECS.broadcastNetworkMessage("ENTITY_DESTROY", id)
            destroyEntitySafe(id)
        end
    end

    destroyAndBroadcast(ECS.getEntitiesWith({"Enemy"}))
    destroyAndBroadcast(ECS.getEntitiesWith({"Bullet"}))
    destroyAndBroadcast(ECS.getEntitiesWith({"Bonus"}))

    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities > 0 then
        local s = ECS.getComponent(scoreEntities[1], "Score")
        s.value = 0
    end

    ECS.sendMessage("RESET_DIFFICULTY", "")
end

-- ============================================================================
-- PLAYER SPAWNING (Server-side)
-- ============================================================================
function NetworkSystem.spawnPlayerForClient(clientId)
    clientId = tonumber(clientId)
    if not clientId then return end

    -- Offset Y based on Client ID to prevent stacking
    local offsetY = (clientId % 4) * 2.0 - 3.0
    local player = Spawns.createPlayer(-8, offsetY, 0, clientId)

    NetworkSystem.clientEntities[clientId] = player
    local session = touchPlayerSession(clientId)
    if session then
        session.entityId = player
    end

    ECS.sendToClient(clientId, "PLAYER_ASSIGN", tostring(player))
    print("[NetworkSystem] Spawned player for client " .. clientId .. " -> entity " .. tostring(player))
end

-- ============================================================================
-- CLIENT ENTITY UPDATE (Client-side)
-- ============================================================================
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
            local pid = tonumber(serverId) or 0
            if pid == 0 then
                local s = tostring(serverId)
                for i = 1, #s do pid = pid + string.byte(s, i) end
            end

            ECS.addComponent(localId, "Mesh", Mesh("assets/models/simple_plane.obj"))

            local colors = {
                {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}, {1.0, 1.0, 0.0},
                {1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}, {1.0, 0.5, 0.0}, {0.5, 0.0, 1.0}
            }
            local colorIndex = ((pid * 7) % #colors) + 1
            local col = colors[colorIndex]
            ECS.addComponent(localId, "Color", Color(col[1], col[2], col[3]))

            t.sx, t.sy, t.sz = getScaleForType(1)
            t.rx, t.ry, t.rz = 0, 0, 0

            ECS.addComponent(localId, "ParticleGenerator", ParticleGenerator(
                -1.0, 0, 0, -1, 0, 0, 0.2, 2.0, 0.5, 50.0, 0.1, 0.0, 0.5, 1.0
            ))

            if serverId == NetworkSystem.myServerId then
                ECS.addComponent(localId, "InputState", { up=false, down=false, left=false, right=false, shoot=false })
                ECS.addComponent(localId, "Player", Player(config.player.speed))
                ECS.addComponent(localId, "Weapon", Weapon(config.player.weaponCooldown))
                ECS.addComponent(localId, "Physic", Physic(1.0, 0.0, false, false))
                ECS.addComponent(localId, "Life", Life(100))
            end

        elseif nType == 2 then
            ECS.addComponent(localId, "Mesh", Mesh("assets/models/player_bullet.obj", "assets/textures/shoot.jpg"))
            ECS.addComponent(localId, "Color", Color(0.0, 1.0, 1.0))
            t.sx, t.sy, t.sz = 0.2, 0.2, 0.2
        elseif nType == 3 then
            ECS.addComponent(localId, "Mesh", Mesh("assets/models/enemy_bullet.obj", "assets/textures/attack.jpg"))
            ECS.addComponent(localId, "Color", Color(1.0, 0.5, 0.0))
            t.sx, t.sy, t.sz = 0.2, 0.2, 0.2
        elseif nType == 99 then
            ECS.addComponent(localId, "Mesh", Mesh("assets/models/Monster_3/motion_1.obj", nil))
            ECS.addComponent(localId, "Color", Color(0.9, 0.1, 0.1))
            ECS.addComponent(localId, "Animation", Animation(8, 0.15, true, "assets/models/Monster_3/motion_"))
            t.sx, t.sy, t.sz = getScaleForType(99)
        elseif nType == 61 or nType == 62 or nType == 63 then
            ECS.addComponent(localId, "Mesh", Mesh("assets/models/powerup_crystal.obj", nil))
            ECS.addComponent(localId, "Tag", Tag({"PowerUp", "Bonus"}))
            if nType == 61 then
                ECS.addComponent(localId, "Color", Color(0.2, 0.8, 1.0))
            elseif nType == 62 then
                ECS.addComponent(localId, "Color", Color(1.0, 0.8, 0.0))
            else
                ECS.addComponent(localId, "Color", Color(1.0, 0.4, 0.2))
            end
            t.sx, t.sy, t.sz = getScaleForType(nType)
        elseif nType >= 4 then
            local configIndex = nType - 3
            local enemyConfigs = {
                [1] = { mesh = "assets/models/Monster_1/motion_1.obj", color = Color(1.0, 0.0, 0.0), frames = 8},
                [2] = { mesh = "assets/models/Monster_2/motion_1.obj", color = Color(0.5, 0.5, 0.5), frames = 3},
                [3] = { mesh = "assets/models/Monster_3/motion_1.obj", color = Color(0.6, 0.4, 0.2), frames = 8}
            }
            local cfg = enemyConfigs[configIndex] or enemyConfigs[3]
            ECS.addComponent(localId, "Mesh", Mesh(cfg.mesh, nil))
            ECS.addComponent(localId, "Color", cfg.color)
            local animBase = "assets/models/Monster_" .. configIndex .. "/motion_"
            ECS.addComponent(localId, "Animation", Animation(cfg.frames, 0.2, true, animBase))
            t.sx, t.sy, t.sz = 0.2, 0.2, 0.2
        end

        NetworkSystem.serverEntities[serverId] = localId
    else
        local t = ECS.getComponent(localId, "Transform")
        if t then
            t.targetX, t.targetY, t.targetZ = nx, ny, nz
            if nType == 1 then
                t.targetRX, t.targetRY, t.targetRZ = 0, 0, 0
                t.rx, t.ry, t.rz = 0, 0, 0
            else
                t.targetRX, t.targetRY, t.targetRZ = nrx, nry, nrz
            end
            t.netVX, t.netVY, t.netVZ = nvx, nvy, nvz
            t.netAge = 0
            t.sx, t.sy, t.sz = getScaleForType(nType)
            if t.x == 0 and t.y == 0 and t.targetX ~= 0 then
                t.x, t.y, t.z = nx, ny, nz
            end
        end
    end

    -- Ensure local player has input components
    if serverId == NetworkSystem.myServerId and localId then
        if not ECS.hasComponent(localId, "InputState") then
            ECS.addComponent(localId, "InputState", { up=false, down=false, left=false, right=false, shoot=false })
            ECS.addComponent(localId, "Player", Player(config.player.speed))
            ECS.addComponent(localId, "Weapon", Weapon(config.player.weaponCooldown))
            ECS.addComponent(localId, "Physic", Physic(1.0, 0.0, true, false))
            ECS.addComponent(localId, "Life", Life(100))
        end
    end
end

-- ============================================================================
-- INIT
-- ============================================================================
function NetworkSystem.init()
    -- ========================================================================
    -- SERVER MODE
    -- ========================================================================
    if ECS.capabilities.hasAuthority and ECS.capabilities.hasNetworkSync then
        print("[NetworkSystem] Server Mode - Game State managed by C++ GameStateMachine")

        -- Create Score Entity
        local scoreEnt = ECS.createEntity()
        ECS.addComponent(scoreEnt, "Score", Score(0))

        -- Client connected - C++ GameStateMachine handles state
        ECS.subscribe("ClientConnected", function(msg)
            local clientId = tonumber(string.match(msg, "^(%d+)"))
            if not clientId then return end
            touchPlayerSession(clientId)
            print("[NetworkSystem] Client connected: " .. clientId)
        end)

        -- Player joined lobby
        ECS.subscribe("PLAYER_JOIN", function(msg)
            local clientId = tonumber(string.match(msg, "^(%d+)"))
            if not clientId then return end
            touchPlayerSession(clientId)
            NetworkSystem.joinedClients[clientId] = true
            print("[NetworkSystem] Player " .. clientId .. " joined lobby")
        end)

        -- Player ready
        ECS.subscribe("PLAYER_READY", function(msg)
            local clientId = tonumber(string.match(msg, "^(%d+)"))
            if not clientId then return end
            touchPlayerSession(clientId)
            NetworkSystem.readyClients[clientId] = true
            print("[NetworkSystem] Player " .. clientId .. " ready")
        end)

        -- GAME_START from C++ GameStateMachine
        ECS.subscribe("GAME_START", function(msg)
            print("[NetworkSystem] GAME_START - Spawning all ready players")
            ECS.isGameRunning = true
            resetLevelToOne()
            NetworkSystem.resetWorldState()

            -- Cleanup any existing player entities
            for clientId, entityId in pairs(NetworkSystem.clientEntities) do
                cleanupPlayerEntity(clientId)
            end

            -- Spawn all ready/joined players
            for clientId in pairs(NetworkSystem.joinedClients) do
                if NetworkSystem.readyClients[clientId] then
                    NetworkSystem.spawnPlayerForClient(clientId)
                end
            end

            local levelNum = resolveCurrentLevel()
            ECS.broadcastNetworkMessage("LEVEL_CHANGE", tostring(levelNum))
            ECS.sendMessage("REQUEST_GAME_STATE_CHANGE", "PLAYING")
        end)

        -- GAME_END from C++ GameStateMachine
        ECS.subscribe("GAME_END", function(msg)
            print("[NetworkSystem] GAME_END - Resetting to lobby")
            ECS.isGameRunning = false

            -- Cleanup all player entities
            for clientId, entityId in pairs(NetworkSystem.clientEntities) do
                cleanupPlayerEntity(clientId)
            end
            NetworkSystem.clientEntities = {}

            -- Reset world
            NetworkSystem.resetWorldState()

            -- Reset ready states (C++ GameStateMachine already reset)
            NetworkSystem.readyClients = {}

            ECS.sendMessage("REQUEST_GAME_STATE_CHANGE", "MENU")
        end)

        -- GAME_WAITING_ROOM from C++ GameStateMachine (back to lobby)
        ECS.subscribe("GAME_WAITING_ROOM", function(msg)
            print("[NetworkSystem] GAME_WAITING_ROOM - Players returned to lobby")
            ECS.isGameRunning = false
            NetworkSystem.readyClients = {}
        end)

        -- LEVEL_CHANGE - Revive dead players and restore HP
        ECS.subscribe("LEVEL_CHANGE", function(level)
            print("[NetworkSystem] LEVEL_CHANGE to level " .. tostring(level) .. " - Reviving all players")

            -- Tell C++ to revive dead players in GameStateMachine
            ECS.sendMessage("RequestRevivePlayers", "")

            -- Respawn any dead players (those without entities)
            for clientId in pairs(NetworkSystem.joinedClients) do
                if not NetworkSystem.clientEntities[clientId] then
                    -- Player was dead, respawn them
                    NetworkSystem.spawnPlayerForClient(clientId)
                    print("[NetworkSystem] Respawned dead player " .. clientId)
                else
                    -- Player alive, restore HP to full
                    local entityId = NetworkSystem.clientEntities[clientId]
                    local life = ECS.getComponent(entityId, "Life")
                    if life then
                        life.amount = life.max or 100
                        ECS.addComponent(entityId, "Life", life)
                        -- Broadcast HP update
                        local net = ECS.getComponent(entityId, "NetworkIdentity")
                        if net and net.uuid then
                            ECS.broadcastNetworkMessage("PLAYER_HP", tostring(net.uuid) .. " " .. tostring(life.amount) .. " " .. tostring(life.max))
                        end
                        print("[NetworkSystem] Restored HP for player " .. clientId)
                    end
                end
            end
        end)

        -- Player left game
        ECS.subscribe("PLAYER_LEAVE", function(msg)
            local clientId = tonumber(string.match(msg, "^(%d+)"))
            if not clientId then return end
            cleanupPlayerEntity(clientId)
            NetworkSystem.readyClients[clientId] = nil
            print("[NetworkSystem] Player " .. clientId .. " left")
        end)

        -- Player died - notify server to update state
        ECS.subscribe("SERVER_PLAYER_DEAD", function(msg)
            local clientId = tonumber(string.match(msg, "^(%d+)"))
            if not clientId then return end
            print("[NetworkSystem] Player " .. clientId .. " died")
            cleanupPlayerEntity(clientId)
            ECS.sendMessage("RequestPlayerDied", tostring(clientId))
        end)

        -- Client reported death
        ECS.subscribe("PLAYER_DIED", function(msg)
            local clientId = tonumber(string.match(msg, "^(%d+)"))
            if not clientId then return end
            print("[NetworkSystem] Player " .. clientId .. " reported death")
            cleanupPlayerEntity(clientId)
            ECS.sendMessage("RequestPlayerDied", tostring(clientId))
        end)

        -- Client disconnected
        ECS.subscribe("ClientDisconnected", function(msg)
            local clientId = tonumber(string.match(msg, "^(%d+)"))
            if not clientId then return end
            cleanupPlayerEntity(clientId)
            NetworkSystem.playerSessions[clientId] = nil
            NetworkSystem.readyClients[clientId] = nil
            NetworkSystem.joinedClients[clientId] = nil
            ECS.broadcastNetworkMessage("PLAYER_LEFT", tostring(clientId) .. " disconnected")
            print("[NetworkSystem] Client " .. clientId .. " disconnected")
        end)

        -- Input handling (server-authoritative)
        ECS.subscribe("INPUT", function(msg)
            local clientId, payload = ECS.splitClientIdAndMessage(msg)
            if clientId and clientId > 0 then
                touchPlayerSession(clientId)
            end

            local data = ECS.unpackMsgPack(payload)
            local key, state = nil, nil
            if data then
                key = data.k
                state = data.s
            else
                local k, s = string.match(payload, "(%w+) (%d)")
                if k then
                    key = k
                    state = tonumber(s)
                end
            end

            if clientId and key and NetworkSystem.clientEntities[clientId] then
                local entityId = NetworkSystem.clientEntities[clientId]
                local input = ECS.getComponent(entityId, "InputState")
                if input then
                    local pressed = (state == 1 or state == true)
                    if key == "UP" or key == "Z" or key == "W" then input.up = pressed end
                    if key == "DOWN" or key == "S" then input.down = pressed end
                    if key == "LEFT" or key == "Q" or key == "A" then input.left = pressed end
                    if key == "RIGHT" or key == "D" then input.right = pressed end
                    if key == "SPACE" then input.shoot = pressed end
                end
            end
        end)

    -- ========================================================================
    -- CLIENT MODE
    -- ========================================================================
    elseif not ECS.capabilities.hasAuthority and ECS.capabilities.hasNetworkSync then
        print("[NetworkSystem] Client Mode - Receiving Network Sync")
        NetworkSystem.clientState = NetworkSystem.CLIENT_STATE.LOBBY

        -- Player assignment
        ECS.subscribe("PLAYER_ASSIGN", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id then
                print("[NetworkSystem] Assigned player ID: " .. id)
                NetworkSystem.myServerId = id
                NetworkSystem.clientDeathReported = false
                NetworkSystem.updateLocalEntity(id, -8, 0, 0, 0, 0, 0, 0, 0, 0, "1")
                ECS.sendNetworkMessage("ACK", "PLAYER_ASSIGN")
            end
        end)

        -- Game starting (transition from LOBBY)
        ECS.subscribe("GAME_STARTING", function(msg)
            print("[NetworkSystem] Game starting...")
            NetworkSystem.clientState = NetworkSystem.CLIENT_STATE.IN_GAME
        end)

        -- Game start
        ECS.subscribe("GAME_START", function(msg)
            print("[NetworkSystem] GAME_START received")
            ECS.sendNetworkMessage("ACK", "GAME_START")
            ECS.isGameRunning = true
            NetworkSystem.clientState = NetworkSystem.CLIENT_STATE.IN_GAME
            NetworkSystem.clientDeathReported = false

            if not NetworkSystem.myServerId then
                ECS.sendNetworkMessage("REQUEST_SPAWN", "1")
            end

            local bgEntities = ECS.getEntitiesWith({"Background"})
            if #bgEntities == 0 then
                local levelNum = resolveCurrentLevel()
                dofile("assets/scripts/space-shooter/levels/Level-" .. levelNum .. ".lua")
                ECS.sendMessage("ShowLevelIntro", tostring(levelNum))
                ScoreSystem.adjustToScreenSize(_G.SCREEN_WIDTH, _G.SCREEN_HEIGHT)
            end
        end)

        -- Game end
        ECS.subscribe("GAME_END", function(msg)
            print("[NetworkSystem] GAME_END received")
            ECS.isGameRunning = false
            NetworkSystem.clientState = NetworkSystem.CLIENT_STATE.LOBBY
            NetworkSystem.myServerId = nil
            NetworkSystem.serverEntities = {}
            NetworkSystem.clientDeathReported = false
            ECS.sendMessage("FORCE_WAITING_ROOM", "game_ended")
        end)

        -- Waiting room (lobby)
        ECS.subscribe("GAME_WAITING_ROOM", function(msg)
            ECS.isGameRunning = false
            NetworkSystem.clientState = NetworkSystem.CLIENT_STATE.LOBBY
            ECS.sendMessage("FORCE_WAITING_ROOM", tostring(msg or ""))
        end)

        -- Return to lobby
        ECS.subscribe("RETURN_TO_LOBBY", function(msg)
            ECS.isGameRunning = false
            NetworkSystem.clientState = NetworkSystem.CLIENT_STATE.LOBBY
            NetworkSystem.clientDeathReported = false
            ECS.sendMessage("FORCE_WAITING_ROOM", tostring(msg or ""))
        end)

        -- Client reset
        ECS.subscribe("CLIENT_RESET", function(msg)
            print("[NetworkSystem] CLIENT_RESET")
            NetworkSystem.myServerId = nil
            NetworkSystem.serverEntities = {}
            NetworkSystem.clientDeathReported = false
            ECS.isGameRunning = false

            local cleanupIds = ECS.getEntitiesWith({"Transform"})
            for _, eid in ipairs(cleanupIds) do
                local tag = ECS.getComponent(eid, "Tag")
                local camera = ECS.getComponent(eid, "Camera")
                local keep = camera ~= nil
                if tag and not keep then
                    for _, t in ipairs(tag.tags) do
                        if t == "MenuEntity" or t == "GameUI" or t == "GameOverEntity" then
                            keep = true
                            break
                        end
                    end
                end
                if not keep then
                    destroyEntitySafe(eid)
                end
            end
        end)

        -- Level change
        ECS.subscribe("LEVEL_CHANGE", function(level)
            local levelNum = tonumber(level) or 1
            local backgroundEntities = ECS.getEntitiesWith({"Background"})
            for _, id in ipairs(backgroundEntities) do
                ECS.destroyEntity(id)
            end
            dofile("assets/scripts/space-shooter/levels/Level-" .. levelNum .. ".lua")
            ECS.sendMessage("ShowLevelIntro", tostring(levelNum))
            ScoreSystem.adjustToScreenSize(_G.SCREEN_WIDTH, _G.SCREEN_HEIGHT)
        end)

        -- Entity position
        ECS.subscribe("ENTITY_POS", function(msg)
            if not ECS.isGameRunning then return end
            local data = ECS.unpackMsgPack(msg)
            if data then
                NetworkSystem.updateLocalEntity(data.id, data.x, data.y, data.z, data.rx, data.ry, data.rz, data.vx, data.vy, data.vz, tostring(data.t))
            else
                local id, x, y, z, rx, ry, rz, vx, vy, vz, typeVal = string.match(msg, "([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+)")
                if id then
                    NetworkSystem.updateLocalEntity(id, x, y, z, rx, ry, rz, vx, vy, vz, typeVal)
                end
            end
        end)

        -- Entity destroy
        ECS.subscribe("ENTITY_DESTROY", function(msg)
            local id = string.match(msg, "([^%s]+)")
            if id and NetworkSystem.serverEntities[id] then
                destroyEntitySafe(NetworkSystem.serverEntities[id])
                NetworkSystem.serverEntities[id] = nil
            end
        end)

        -- Enemy dead (with explosion)
        ECS.subscribe("ENEMY_DEAD", function(msg)
            if not ECS.isGameRunning then return end
            local id, x, y, z = string.match(msg, "([^%s]+) ([^%s]+) ([^%s]+) ([^%s]+)")
            if id then
                if x and y and z then
                    Spawns.createExplosion(tonumber(x), tonumber(y), tonumber(z))
                end
                if NetworkSystem.serverEntities[id] then
                    destroyEntitySafe(NetworkSystem.serverEntities[id])
                    NetworkSystem.serverEntities[id] = nil
                end
            end
        end)

        -- Score update
        ECS.subscribe("GAME_SCORE", function(msg)
            local scoreVal = tonumber(msg)
            if scoreVal then
                CurrentScore = scoreVal
                local scoreEntities = ECS.getEntitiesWith({"Score"})
                if #scoreEntities > 0 then
                    local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
                    scoreComp.value = scoreVal
                else
                    local scoreEntity = ECS.createEntity()
                    ECS.addComponent(scoreEntity, "Score", Score(scoreVal))
                end
            end
        end)

        -- Player HP
        ECS.subscribe("PLAYER_HP", function(msg)
            local id, hpStr, maxStr = string.match(msg, "([^%s]+)%s+([^%s]+)%s+([^%s]+)")
            if not id or id ~= tostring(NetworkSystem.myServerId) then return end

            local hp = tonumber(hpStr)
            local maxHp = tonumber(maxStr)
            if not hp or not maxHp then return end

            local localId = NetworkSystem.serverEntities[id]
            if not localId then return end

            local life = ECS.getComponent(localId, "Life") or Life(maxHp)
            life.amount = hp
            life.max = maxHp
            ECS.addComponent(localId, "Life", life)

            if hp <= 0 and not NetworkSystem.clientDeathReported then
                NetworkSystem.clientDeathReported = true
                ECS.sendNetworkMessage("PLAYER_DIED", "hp_zero")
            end
        end)

        -- Game over
        ECS.subscribe("GAME_OVER", function(msg)
            if not NetworkSystem.clientDeathReported then
                NetworkSystem.clientDeathReported = true
                ECS.sendNetworkMessage("PLAYER_DIED", "game_over")
            end
        end)

        -- Sound events from server
        ECS.subscribe("PLAY_SOUND", function(msg)
            ECS.sendMessage("SoundPlay", msg)
        end)

        ECS.subscribe("STOP_MUSIC", function(msg)
            ECS.sendMessage("MusicStop", msg)
        end)
    end
end

-- ============================================================================
-- UPDATE
-- ============================================================================
function NetworkSystem.update(dt)
    -- ========================================================================
    -- CLIENT-SIDE UPDATE
    -- ========================================================================
    if not ECS.capabilities.hasAuthority and ECS.capabilities.hasNetworkSync then
        -- Send heartbeat during IN_GAME
        if NetworkSystem.clientState == NetworkSystem.CLIENT_STATE.IN_GAME then
            NetworkSystem.heartbeatTimer = NetworkSystem.heartbeatTimer + dt
            if NetworkSystem.heartbeatTimer >= NetworkSystem.heartbeatInterval then
                NetworkSystem.heartbeatTimer = 0
                ECS.sendNetworkMessage("HEARTBEAT", "ping")
            end
        end

        -- Entity interpolation
        if ECS.isGameRunning then
            local entities = ECS.getEntitiesWith({"Transform"})
            local lerpSpeed = 18.0

            for _, id in ipairs(entities) do
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
                        t.rx = t.rx + (t.targetRX - t.rx) * lerpSpeed * dt
                        t.ry = t.ry + (t.targetRY - t.ry) * lerpSpeed * dt
                        t.rz = t.rz + (t.targetRZ - t.rz) * lerpSpeed * dt
                    end
                else
                    -- Reconciliation for local player
                    local t = ECS.getComponent(id, "Transform")
                    if t and t.targetX then
                        local dx = t.x - t.targetX
                        local dy = t.y - t.targetY
                        local distSq = dx*dx + dy*dy
                        if distSq > 4.0 then
                            local correctionSpeed = 5.0 * dt
                            t.x = t.x + (t.targetX - t.x) * correctionSpeed
                            t.y = t.y + (t.targetY - t.y) * correctionSpeed
                        end
                    end
                end
            end
        end
        return
    end

    -- ========================================================================
    -- SERVER-SIDE UPDATE
    -- ========================================================================
    if not ECS.capabilities.hasAuthority or not ECS.capabilities.hasNetworkSync then
        return
    end

    if not ECS.isGameRunning then
        return
    end

    NetworkSystem.broadcastTimer = NetworkSystem.broadcastTimer + dt
    if NetworkSystem.broadcastTimer < NetworkSystem.broadcastInterval then return end
    NetworkSystem.broadcastTimer = 0
    NetworkSystem.tickCounter = (NetworkSystem.tickCounter + 1) % 30

    -- Broadcast player positions
    local players = ECS.getEntitiesWith({"Player", "Transform"})
    for _, id in ipairs(players) do
        local t = ECS.getComponent(id, "Transform")
        local phys = ECS.getComponent(id, "Physic")
        if ECS.broadcastBinary then
            ECS.broadcastBinary("ENTITY_POS", buildStateData(id, t, phys, 1))
        end
    end

    -- Broadcast bullets (every 2nd tick)
    if NetworkSystem.tickCounter % 2 == 0 then
        local bullets = ECS.getEntitiesWith({"Bullet", "Transform"})
        for _, id in ipairs(bullets) do
            local t = ECS.getComponent(id, "Transform")
            local phys = ECS.getComponent(id, "Physic")
            local tagComp = ECS.getComponent(id, "Tag")
            local isEnemyBullet = false
            if tagComp and tagComp.tags then
                for _, tag in ipairs(tagComp.tags) do
                    if tag == "EnemyBullet" then isEnemyBullet = true break end
                end
            end
            local typeNum = isEnemyBullet and 3 or 2
            if ECS.broadcastBinary then
                ECS.broadcastBinary("ENTITY_POS", buildStateData(id, t, phys, typeNum))
            end
        end
    end

    -- Broadcast enemies (every 3rd tick)
    if NetworkSystem.tickCounter % 3 == 0 then
        local enemies = ECS.getEntitiesWith({"Enemy", "Transform"})
        for _, id in ipairs(enemies) do
            local t = ECS.getComponent(id, "Transform")
            local phys = ECS.getComponent(id, "Physic")
            if ECS.broadcastBinary then
                ECS.broadcastBinary("ENTITY_POS", buildStateData(id, t, phys, 4))
            end
        end
    end

    -- Broadcast power-ups (every 2nd tick)
    if NetworkSystem.tickCounter % 2 == 0 then
        local powerUps = ECS.getEntitiesWith({"Bonus", "Transform"})
        for _, id in ipairs(powerUps) do
            local t = ECS.getComponent(id, "Transform")
            local phys = ECS.getComponent(id, "Physic")
            if ECS.broadcastBinary then
                ECS.broadcastBinary("ENTITY_POS", buildStateData(id, t, phys, 61))
            end
        end
    end

    -- Broadcast score (every 10th tick)
    if NetworkSystem.tickCounter % 10 == 0 then
        local scoreEntities = ECS.getEntitiesWith({"Score"})
        if #scoreEntities > 0 then
            local s = ECS.getComponent(scoreEntities[1], "Score")
            ECS.broadcastNetworkMessage("GAME_SCORE", tostring(s.value))
        end
    end

    -- Broadcast player HP (every 3rd tick)
    if NetworkSystem.tickCounter % 3 == 0 then
        local playerEntities = ECS.getEntitiesWith({"Player", "Life", "NetworkIdentity"})
        for _, id in ipairs(playerEntities) do
            local life = ECS.getComponent(id, "Life")
            local net = ECS.getComponent(id, "NetworkIdentity")
            if life and net and net.uuid then
                ECS.broadcastNetworkMessage("PLAYER_HP", tostring(net.uuid) .. " " .. tostring(life.amount or 0) .. " " .. tostring(life.max or 100))
            end
        end
    end
end

ECS.registerSystem(NetworkSystem)
