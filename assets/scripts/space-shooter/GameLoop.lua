-- ========================================================
-- R-TYPE 3D - FINAL GAME LOOP (With Collisions)
-- ========================================================

local Game = {
    playerID = -1,
    bullets = {},
    enemies = {},

    -- Config
    speed = 400,
    bulletSpeed = 800,
    enemySpeed = 200,
    fireRate = 0.25,
    timeSinceFire = 0,
    enemySpawnTimer = 0,

    -- Collision Radii (approx)
    playerRadius = 30,
    enemyRadius = 20,
    bulletRadius = 5,

    -- Constants
    RAD_90 = -math.pi / 2,
    RAD_15 = math.pi / 12
}

function Game.init()
    ECS.log("🚀 Lancement de R-Type 3D (No Physics Engine)...")

    -- 1. Camera
    local cam = ECS.createEntity()
    ECS.addComponent(cam, "Transform", { x = 400, y = 300, z = 520 })
    ECS.addComponent(cam, "Camera", {})

    -- 2. Player
    Game.playerID = ECS.createEntity()
    ECS.addComponent(Game.playerID, "Transform", {
        x = 100, y = 300, z = 0,
        rx = 0, ry = Game.RAD_90, rz = 0,
        scale = 10.0
    })
    ECS.createMesh(Game.playerID, "assets/models/fighter.obj")
    if ECS.setTexture then ECS.setTexture(Game.playerID, "assets/textures/plane_texture.png") end

    -- 3. HUD
    local txt = ECS.createEntity()
    ECS.addComponent(txt, "Transform", { x = 10, y = 10, scale = 1.0 })
    ECS.createText(txt, "Joueur: ZQSD | Tir: Espace", "assets/fonts/arial.ttf", 24, true)

    ECS.log("Vaisseau prêt (ID: " .. Game.playerID .. ")")
end

function Game.update(dt)
    if Game.playerID == -1 then return end

    Game.handlePlayer(dt)
    Game.handleSpawning(dt)
    Game.updateBullets(dt)
    Game.updateEnemies(dt)

    -- 5. Collision Detection
    Game.handleCollisions()

    ECS.syncToRenderer()
end

function Game.handleCollisions()
    -- 1. Bullets vs Enemies
    for i = #Game.bullets, 1, -1 do
        local bID = Game.bullets[i]
        local bT = ECS.getComponent(bID, "Transform")

        if bT then
            for j = #Game.enemies, 1, -1 do
                local eID = Game.enemies[j]
                local eT = ECS.getComponent(eID, "Transform")

                if eT then
                    if Game.checkCollision(bT, eT, Game.bulletRadius, Game.enemyRadius) then
                        ECS.log("💥 BOOM! Ennemi détruit par tir.")

                        -- Destroy both
                        ECS.destroyEntity(bID)
                        ECS.destroyEntity(eID)

                        -- Remove from tables
                        table.remove(Game.bullets, i)
                        table.remove(Game.enemies, j)

                        -- Break enemy loop since bullet is gone
                        break
                    end
                end
            end
        end
    end

    -- 2. Player vs Enemies
    local pT = ECS.getComponent(Game.playerID, "Transform")
    if pT then
        for j = #Game.enemies, 1, -1 do
            local eID = Game.enemies[j]
            local eT = ECS.getComponent(eID, "Transform")

            if eT then
                if Game.checkCollision(pT, eT, Game.playerRadius, Game.enemyRadius) then
                    ECS.log("💀 GAME OVER! Collision vaisseau.")
                    ECS.destroyEntity(Game.playerID)
                    Game.playerID = -1
                    return -- Stop update
                end
            end
        end
    end
end

-- Simple Circle Collision
function Game.checkCollision(t1, t2, r1, r2)
    local dx = t1.x - t2.x
    local dy = t1.y - t2.y
    local distSq = dx*dx + dy*dy
    local radii = r1 + r2
    return distSq < (radii * radii)
end

function Game.handlePlayer(dt)
    local t = ECS.getComponent(Game.playerID, "Transform")
    local moveY = 0
    local moveX = 0

    if ECS.isKeyPressed("Z") or ECS.isKeyPressed("Up") then moveY = 1 end
    if ECS.isKeyPressed("S") or ECS.isKeyPressed("Down") then moveY = -1 end
    if ECS.isKeyPressed("Q") or ECS.isKeyPressed("Left") then moveX = -1 end
    if ECS.isKeyPressed("D") or ECS.isKeyPressed("Right") then moveX = 1 end

    t.y = t.y + (moveY * Game.speed * dt)
    t.x = t.x + (moveX * Game.speed * dt)

    if t.x < 50 then t.x = 50 end
    if t.x > 750 then t.x = 750 end
    if t.y < 50 then t.y = 50 end
    if t.y > 550 then t.y = 550 end

    if moveY == 1 then t.rx = -Game.RAD_15
    elseif moveY == -1 then t.rx = Game.RAD_15
    else t.rx = 0 end

    ECS.updateComponent(Game.playerID, "Transform", t)

    Game.timeSinceFire = Game.timeSinceFire + dt
    if ECS.isKeyPressed("Space") and Game.timeSinceFire > Game.fireRate then
        Game.spawnBullet(t.x + 40, t.y)
        Game.timeSinceFire = 0
    end
end

function Game.spawnBullet(x, y)
    local id = ECS.createEntity()
    ECS.addComponent(id, "Transform", {
        x = x, y = y, z = 0,
        rx = 0, ry = 0, rz = 0,
        scale = 5.0
    })
    ECS.createMesh(id, "assets/models/cube.obj")
    table.insert(Game.bullets, id)
end

function Game.spawnEnemy()
    local id = ECS.createEntity()
    local randY = math.random(50, 550)

    ECS.addComponent(id, "Transform", {
        x = 900, y = randY, z = 0,
        rx = 0, ry = -Game.RAD_90, rz = 0,
        scale = 30.0
    })
    ECS.createMesh(id, "assets/models/simple_plane.obj")
    if ECS.setTexture then ECS.setTexture(id, "assets/textures/plane_texture.png") end

    table.insert(Game.enemies, id)
end

function Game.handleSpawning(dt)
    Game.enemySpawnTimer = Game.enemySpawnTimer + dt
    if Game.enemySpawnTimer > 2.0 then
        Game.spawnEnemy()
        Game.enemySpawnTimer = 0
    end
end

function Game.updateBullets(dt)
    for i = #Game.bullets, 1, -1 do
        local id = Game.bullets[i]
        local t = ECS.getComponent(id, "Transform")

        if t then
            t.x = t.x + (Game.bulletSpeed * dt)
            if t.x > 850 then
                ECS.destroyEntity(id)
                table.remove(Game.bullets, i)
            else
                t.rx = t.rx + (5.0 * dt)
                ECS.updateComponent(id, "Transform", t)
            end
        else
            table.remove(Game.bullets, i)
        end
    end
end

function Game.updateEnemies(dt)
    for i = #Game.enemies, 1, -1 do
        local id = Game.enemies[i]
        local t = ECS.getComponent(id, "Transform")

        if t then
            t.x = t.x - (Game.enemySpeed * dt)
            if t.x < -100 then
                ECS.destroyEntity(id)
                table.remove(Game.enemies, i)
            else
                ECS.updateComponent(id, "Transform", t)
            end
        else
            table.remove(Game.enemies, i)
        end
    end
end

ECS.registerSystem(Game)