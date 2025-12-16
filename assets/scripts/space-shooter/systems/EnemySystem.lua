local config = dofile("assets/scripts/space-shooter/config.lua")
local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")

local EnemySystem = {}

EnemySystem.spawnTimer = 0
EnemySystem.spawnInterval = config.enemy.spawnInterval or 2.0
EnemySystem.gameTime = 0
EnemySystem.baseSpawnInterval = config.enemy.spawnInterval or 2.0
EnemySystem.baseSpeed = config.enemy.speed or 5.0

local function getGameStateState()
    local entities = ECS.getEntitiesWith({"GameState", "ServerAuthority"})
    if #entities == 0 then
        entities = ECS.getEntitiesWith({"GameState"})
    end
    if #entities == 0 then
        return nil
    end
    local gs = ECS.getComponent(entities[1], "GameState")
    return gs and gs.state or nil
end

function EnemySystem.init()
    print("[EnemySystem] Initialized")
    EnemySystem.resetDifficulty() -- Clear state on init
end

function EnemySystem.update(dt)
    -- Only run on authoritative instances (Server or Solo)
    if not ECS.capabilities.hasAuthority then return end
    if not ECS.isGameRunning then return end
    local gsState = getGameStateState()
    if gsState and gsState ~= "PLAYING" then return end

    -- Wait for players to join before simulating enemies
    local players = ECS.getEntitiesWith({"Player"})
    -- print("DEBUG: EnemySystem found " .. #players .. " players") -- Uncomment for spam debug
    if #players == 0 then
        return
    end

    EnemySystem.spawnTimer = EnemySystem.spawnTimer + dt
    if EnemySystem.spawnTimer >= EnemySystem.spawnInterval then
        EnemySystem.spawnTimer = 0
        EnemySystem.spawnEnemy()
    end

    -- Clean up enemies that go off screen (left)
    local enemies = ECS.getEntitiesWith({"Enemy", "Transform"})
    for _, id in ipairs(enemies) do
        local t = ECS.getComponent(id, "Transform")
        if t.x < -20 then
            -- Decrease score when enemy escapes
            local scoreEntities = ECS.getEntitiesWith({"Score"})
            if #scoreEntities > 0 then
                local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
                scoreComp.value = scoreComp.value - 20
                if scoreComp.value < 0 then
                    scoreComp.value = 0
                end
            end
            ECS.destroyEntity(id)
        end
    end
end

function EnemySystem.spawnEnemy()
    local y = math.random(-5, 5)
    local x = 25 

    local difficultyMultiplier = 1.0 + (EnemySystem.gameTime / 30.0)
    local speed = EnemySystem.baseSpeed * difficultyMultiplier
    if config.enemy.maxSpeed and speed > config.enemy.maxSpeed then speed = config.enemy.maxSpeed end
    
    -- Debug Difficulty
    -- print("Spawning Enemy. Time: " .. EnemySystem.gameTime .. " Speed: " .. speed)

    Spawns.spawnEnemy(x, y, speed)
end

-- Hook for NetworkSystem to reset
function EnemySystem.resetDifficulty()
    print("[EnemySystem] Resetting Difficulty")
    EnemySystem.gameTime = 0
    EnemySystem.spawnTimer = 0
    EnemySystem.spawnInterval = EnemySystem.baseSpawnInterval
end

-- Subscribe to global event if possible, or poll?
-- LuaECSManager doesn't allow calling System function from another System easily without event.
-- Let's stick to simple var reset if reloading script, but script stays loaded on server.
-- We can add a subscription to "RESET_DIFFICULTY"
ECS.subscribe("RESET_DIFFICULTY", function(msg)
    EnemySystem.resetDifficulty()
end)

ECS.registerSystem(EnemySystem)
