local EnemySystem = {}

EnemySystem.spawnTimer = 0
EnemySystem.spawnInterval = 2.0 -- Spawn every 2 seconds
EnemySystem.gameTime = 0
EnemySystem.baseSpawnInterval = 2.0
EnemySystem.baseSpeed = 5.0

function EnemySystem.init()
    print("[EnemySystem] Initialized")
end

function EnemySystem.update(dt)
    EnemySystem.gameTime = EnemySystem.gameTime + dt

    -- Increase difficulty over time
    local difficultyMultiplier = 1.0 + (EnemySystem.gameTime / 30.0) -- Every 30 seconds increases difficulty
    EnemySystem.spawnInterval = EnemySystem.baseSpawnInterval / difficultyMultiplier
    if EnemySystem.spawnInterval < 0.5 then
        EnemySystem.spawnInterval = 0.5 -- Minimum spawn interval
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
        if t.x < -15 then
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
    local x = 15 -- Spawn off-screen right

    local difficultyMultiplier = 1.0 + (EnemySystem.gameTime / 30.0)
    local speed = EnemySystem.baseSpeed * difficultyMultiplier
    if speed > 15.0 then
        speed = 15.0 -- Cap max speed
    end

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

ECS.registerSystem(EnemySystem)
