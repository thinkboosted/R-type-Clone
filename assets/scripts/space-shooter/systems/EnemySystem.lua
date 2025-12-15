local EnemySystem = {}

EnemySystem.spawnTimer = 0
EnemySystem.spawnInterval = 2.0 -- Spawn every 2 seconds
EnemySystem.gameTime = 0
EnemySystem.baseSpawnInterval = 2.0
EnemySystem.baseSpeed = 5.0

function EnemySystem.init()
    print("[EnemySystem] Initialized")
    EnemySystem.resetDifficulty() -- Clear state on init
end

function EnemySystem.update(dt)
    if not ECS.isServer() and not ECS.isLocal then return end
    if not ECS.isGameRunning then return end

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
    if speed > 15.0 then
        speed = 15.0
    end
    
    -- Debug Difficulty
    -- print("Spawning Enemy. Time: " .. EnemySystem.gameTime .. " Speed: " .. speed)

    local enemy = ECS.createEntity()
    ECS.addComponent(enemy, "Transform", Transform(x, y, 0))
    ECS.addComponent(enemy, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(enemy, "Collider", Collider("Box", {1, 1, 1}))
    ECS.addComponent(enemy, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(enemy, "Enemy", Enemy(speed))
    ECS.addComponent(enemy, "Life", Life(1))
    ECS.addComponent(enemy, "Color", Color(1.0, 0.0, 0.0))

    local p = ECS.getComponent(enemy, "Physic")
    p.vx = -speed
end

-- Hook for NetworkSystem to reset
function EnemySystem.resetDifficulty()
    print("[EnemySystem] Resetting Difficulty")
    EnemySystem.gameTime = 0
    EnemySystem.spawnTimer = 0
end

-- Subscribe to global event if possible, or poll?
-- LuaECSManager doesn't allow calling System function from another System easily without event.
-- Let's stick to simple var reset if reloading script, but script stays loaded on server.
-- We can add a subscription to "RESET_DIFFICULTY"
ECS.subscribe("RESET_DIFFICULTY", function(msg)
    EnemySystem.resetDifficulty()
end)

ECS.registerSystem(EnemySystem)
