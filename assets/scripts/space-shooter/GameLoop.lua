-- ========================================================
-- R-TYPE: GAMEPLAY LOOP
-- ========================================================

local Game = {}

-- IDs & Listes
local playerID = -1
local bullets = {} -- Liste des IDs des balles actives
local enemies = {} -- Liste des IDs des ennemis actifs

-- Config
local speed = 400
local bulletSpeed = 800
local enemySpeed = 200
local fireRate = 0.25
local timeSinceFire = 0
local enemySpawnTimer = 0

-- Constantes Maths
local RAD_90 = -math.pi / 2
local RAD_15 = math.pi / 12

function Game.init()
    ECS.log("🚀 Lancement du Gameplay R-Type...")

    -- 1. Camera
    local cam = ECS.createEntity()
    ECS.addComponent(cam, "Transform", { x = 400, y = 300, z = 520 })
    ECS.addComponent(cam, "Camera", {})

    -- 2. Joueur
    playerID = ECS.createEntity()
    ECS.addComponent(playerID, "Transform", {
        x = 100, y = 300, z = 0,
        rx = 0, ry = RAD_90, rz = 0,
        scale = 10.0
    })
    ECS.createMesh(playerID, "assets/models/fighter.obj")

    if ECS.setTexture then
        ECS.setTexture(playerID, "assets/textures/plane_texture.png")
    end

    -- 3. HUD
    local txt = ECS.createEntity()
    ECS.addComponent(txt, "Transform", { x = 10, y = 10, scale = 1.0 })
    ECS.createText(txt, "Joueur: ZQSD | Tir: Espace", "assets/fonts/arial.ttf", 24, true)

    ECS.log("Systèmes armés. Prêt au combat.")
end

function Game.update(dt)
    if playerID == -1 then return end

    -- ==============================
    -- 1. JOUEUR (Mouvement)
    -- ==============================
    local t = ECS.getComponent(playerID, "Transform")
    local moveY = 0
    local moveX = 0

    if ECS.isKeyPressed("Z") or ECS.isKeyPressed("Up") then moveY = 1 end
    if ECS.isKeyPressed("S") or ECS.isKeyPressed("Down") then moveY = -1 end
    if ECS.isKeyPressed("Q") or ECS.isKeyPressed("Left") then moveX = -1 end
    if ECS.isKeyPressed("D") or ECS.isKeyPressed("Right") then moveX = 1 end

    t.y = t.y + (moveY * speed * dt)
    t.x = t.x + (moveX * speed * dt)

    -- Clamp (Limites écran)
    if t.x < 50 then t.x = 50 end
    if t.x > 750 then t.x = 750 end
    if t.y < 50 then t.y = 50 end
    if t.y > 550 then t.y = 550 end

    -- Effet de Roll
    if moveY == 1 then t.rx = -RAD_15
    elseif moveY == -1 then t.rx = RAD_15
    else t.rx = 0 end

    ECS.updateComponent(playerID, "Transform", t)

    -- ==============================
    -- 2. TIR (Spawn Bullets)
    -- ==============================
    timeSinceFire = timeSinceFire + dt
    if ECS.isKeyPressed("Space") and timeSinceFire > fireRate then
        spawnBullet(t.x + 60, t.y) -- Spawn un peu devant le vaisseau
        timeSinceFire = 0
    end

    -- ==============================
    -- 3. ENNEMIS (Spawn)
    -- ==============================
    enemySpawnTimer = enemySpawnTimer + dt
    if enemySpawnTimer > 2.0 then -- Un ennemi toutes les 2 sec
        spawnEnemy()
        enemySpawnTimer = 0
    end

    -- ==============================
    -- 4. MISE À JOUR ENTITÉS
    -- ==============================
    updateBullets(dt)
    updateEnemies(dt)

    -- Synchro finale vers le C++
    ECS.syncToRenderer()
end

-- --- HELPERS ---

function spawnBullet(x, y)
    local id = ECS.createEntity()
    -- Petite taille, rotation 90 pour être dans l'axe
    ECS.addComponent(id, "Transform", { x = x, y = y, z = 0, scale = 2.0, rx=0, ry=RAD_90, rz=0 })
    -- Utilise un cube ou un modèle de missile si tu as
    ECS.createMesh(id, "cube")
    table.insert(bullets, id)
end

function updateBullets(dt)
    for i = #bullets, 1, -1 do
        local id = bullets[i]
        local t = ECS.getComponent(id, "Transform")

        -- Avance vers la droite
        t.x = t.x + (bulletSpeed * dt)

        -- Si sort de l'écran -> Supprimer (TODO: Binding destroyEntity)
        if t.x > 900 then
            -- ECS.destroyEntity(id) -- À implémenter en C++ !
            table.remove(bullets, i)
        else
            ECS.updateComponent(id, "Transform", t)
        end
    end
end

function spawnEnemy(dt)
    local id = ECS.createEntity()
    local randY = math.random(50, 550)
    -- Spawn à droite, regarde vers la gauche (-90 deg)
    ECS.addComponent(id, "Transform", { x = 900, y = randY, z = 0, scale = 8.0, rx=0, ry=-RAD_90, rz=0 })
    ECS.createMesh(id, "assets/models/fighter.obj") -- Réutilise le fighter pour l'instant (ennemi)
    if ECS.setTexture then ECS.setTexture(id, "assets/textures/plane_texture.png") end
    table.insert(enemies, id)
end

function updateEnemies(dt)
    for i = #enemies, 1, -1 do
        local id = enemies[i]
        local t = ECS.getComponent(id, "Transform")

        -- Avance vers la gauche
        t.x = t.x - (enemySpeed * dt)

        if t.x < -100 then
            table.remove(enemies, i)
        else
            ECS.updateComponent(id, "Transform", t)
        end
    end
end

ECS.registerSystem(Game)