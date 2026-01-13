-- ========================================================
-- R-TYPE 3D - FINAL GAME LOOP (With Vec3 & Sound)
-- ========================================================

local Game = {
    playerID = -1,
    bullets = {},
    enemies = {},

    speed = 400,
    bulletSpeed = 800,
    enemySpeed = 200,
    fireRate = 0.25,
    timeSinceFire = 0,
    enemySpawnTimer = 0,

    playerRadius = 30,
    enemyRadius = 25,
    bulletRadius = 15,

    RAD_90 = -math.pi / 2,
    RAD_15 = math.pi / 12
}

function Game.init()
    ECS.log("🚀 Lancement de R-Type 3D (Sound Edition)...")

    -- 1. Camera
    local cam = ECS.createEntity()
    ECS.addComponent(cam, "Transform", { x = 400, y = 300, z = 520 })
    ECS.addComponent(cam, "Camera", {})

    -- 2. Player
    Game.playerID = ECS.createEntity()
    ECS.addComponent(Game.playerID, "Transform", {
        x = 100, y = 300, z = 0,
        rx = 0, ry = Game.RAD_90, rz = 0,
        scale = 20.0
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
    Game.handleCollisions()

    ECS.syncToRenderer()
end

function Game.handleCollisions()
    -- Bullets vs Enemies
    for i = #Game.bullets, 1, -1 do
        local bID = Game.bullets[i]
        local bT = ECS.getComponent(bID, "Transform")

        if bT then
            local bPos = Vec3.new(bT.x, bT.y, bT.z)

            for j = #Game.enemies, 1, -1 do
                local eID = Game.enemies[j]
                local eT = ECS.getComponent(eID, "Transform")

                if eT then
                    local ePos = Vec3.new(eT.x, eT.y, eT.z)
                    local dist = (bPos - ePos):length()

                    if dist < (Game.bulletRadius + Game.enemyRadius) then
                        -- PLAY SOUND: Explosion
                        ECS.playSound("effects/explosion.wav")

                        ECS.destroyEntity(bID)
                        ECS.destroyEntity(eID)
                        table.remove(Game.bullets, i)
                        table.remove(Game.enemies, j)
                        break
                    end
                end
            end
        end
    end

    -- Player vs Enemies
    local pT = ECS.getComponent(Game.playerID, "Transform")
    if pT then
        local pPos = Vec3.new(pT.x, pT.y, pT.z)
        for j = #Game.enemies, 1, -1 do
            local eID = Game.enemies[j]
            local eT = ECS.getComponent(eID, "Transform")
            if eT then
                local ePos = Vec3.new(eT.x, eT.y, eT.z)
                if (pPos - ePos):length() < (Game.playerRadius + Game.enemyRadius) then
                    -- PLAY SOUND: Game Over
                    ECS.playSound("effects/gameover.wav")

                    ECS.log("💀 GAME OVER!")
                    ECS.destroyEntity(Game.playerID)
                    Game.playerID = -1
                    return
                end
            end
        end
    end
end

function Game.handlePlayer(dt)
    local t = ECS.getComponent(Game.playerID, "Transform")
    local pos = Vec3.new(t.x, t.y, t.z)

    local moveDir = Vec3.new(0, 0, 0)

    if ECS.isKeyPressed("Z") or ECS.isKeyPressed("Up") then moveDir.y = 1 end
    if ECS.isKeyPressed("S") or ECS.isKeyPressed("Down") then moveDir.y = -1 end
    if ECS.isKeyPressed("Q") or ECS.isKeyPressed("Left") then moveDir.x = -1 end
    if ECS.isKeyPressed("D") or ECS.isKeyPressed("Right") then moveDir.x = 1 end

    if moveDir:length() > 0 then
        moveDir = moveDir:normalize()
    end

    pos = pos + (moveDir * Game.speed * dt)

    -- Clamp
    if pos.x < 50 then pos.x = 50 end
    if pos.x > 750 then pos.x = 750 end
    if pos.y < 50 then pos.y = 50 end
    if pos.y > 550 then pos.y = 550 end

    t.x = pos.x
    t.y = pos.y
    t.z = pos.z

    if moveDir.y > 0 then t.rx = -Game.RAD_15
    elseif moveDir.y < 0 then t.rx = Game.RAD_15
    else t.rx = 0 end

    ECS.updateComponent(Game.playerID, "Transform", t)

    -- Fire
    Game.timeSinceFire = Game.timeSinceFire + dt
    if ECS.isKeyPressed("Space") and Game.timeSinceFire > Game.fireRate then
        -- PLAY SOUND: Laser
        ECS.playSound("effects/laser.wav")

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