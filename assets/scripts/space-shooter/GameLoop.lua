-- ========================================================
-- R-TYPE
-- ========================================================

local Game = {}
local playerID = -1
local speed = 400
local RAD_90 = -math.pi / 2
local RAD_15 = math.pi / 12 -- 15 degrees in radians

function Game.init()
    ECS.log("🚀 Lancement de R-Type 3D...")

    local cam = ECS.createEntity()
    ECS.addComponent(cam, "Transform", { x = 400, y = 300, z = 520 })
    ECS.addComponent(cam, "Camera", {})

    playerID = ECS.createEntity()

    ECS.addComponent(playerID, "Transform", {
        x = 100, y = 300, z = 0,
        rx = 0, ry = RAD_90, rz = 0,
        scale = 10.0
    })
    local modelPath = "assets/models/fighter.obj"
    ECS.createMesh(playerID, modelPath)

    if ECS.setTexture then
        ECS.setTexture(playerID, "assets/textures/plane_texture.png")
    else
        ECS.log("⚠️ ECS.setTexture non disponible")
    end

    local txt = ECS.createEntity()
    ECS.addComponent(txt, "Transform", { x = 10, y = 10, scale = 1.0 })
    ECS.createText(txt, "Joueur: ZQSD | Tir: Espace", "assets/fonts/arial.ttf", 24, true)

    ECS.log("Vaisseau créé (ID: " .. playerID .. ")")
end

function Game.update(dt)
    if playerID == -1 then return end

    local t = ECS.getComponent(playerID, "Transform")
    local moveY = 0
    local moveX = 0

    if ECS.isKeyPressed("Z") or ECS.isKeyPressed("Up") then moveY = 1 end
    if ECS.isKeyPressed("S") or ECS.isKeyPressed("Down") then moveY = -1 end
    if ECS.isKeyPressed("Q") or ECS.isKeyPressed("Left") then moveX = -1 end
    if ECS.isKeyPressed("D") or ECS.isKeyPressed("Right") then moveX = 1 end

    t.y = t.y + (moveY * speed * dt)
    t.x = t.x + (moveX * speed * dt)

    if t.x < 0 then t.x = 0 end
    if t.x > 800 then t.x = 800 end
    if t.y < 0 then t.y = 0 end
    if t.y > 600 then t.y = 600 end

    if moveY == 1 then t.rx = -RAD_15
    elseif moveY == -1 then t.rx = RAD_15
    else t.rx = 0 end

    ECS.updateComponent(playerID, "Transform", t)
    ECS.syncToRenderer()
end

ECS.registerSystem(Game)