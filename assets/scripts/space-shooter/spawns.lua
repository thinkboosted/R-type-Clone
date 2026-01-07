local config = dofile("assets/scripts/space-shooter/config.lua")
local Spawns = {}

local function hasRendering()
    return ECS.capabilities and ECS.capabilities.hasRendering
end

function Spawns.createPlayer(x, y, z, clientId)
    local e = ECS.createEntity()
    local idNum = tonumber(clientId) or 0
    print("[Spawns] Creating Player: " .. e .. " (Client " .. idNum .. ")")

    ECS.addComponent(e, "Transform", Transform(x, y, z, 0, 0, 0))
    ECS.addComponent(e, "Tag", Tag({"Player"}))
    ECS.addComponent(e, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(e, "Collider", Collider("BOX", {1, 1, 1}))
    ECS.addComponent(e, "Player", Player(config.player.speed))
    ECS.addComponent(e, "Weapon", Weapon(0.2))
    ECS.addComponent(e, "Life", Life(100))
    ECS.addComponent(e, "InputState", InputState())

    if hasRendering() then
        ECS.addComponent(e, "Mesh", Mesh("assets/models/simple_plane.obj", "assets/textures/plane_texture.png"))
        ECS.addComponent(e, "Color", Color(1.0, 1.0, 1.0))
        ECS.addComponent(e, "ParticleGenerator", ParticleGenerator(-1,0,0,-1,0,0,0.2,2,0.5,50,0.1,0,0.5,1))
    end

    return e
end

function Spawns.createExplosion(x, y, z)
    if not hasRendering() then return end
    local e = ECS.createEntity()
    ECS.addComponent(e, "Transform", Transform(x, y, z))
    ECS.addComponent(e, "ParticleGenerator", ParticleGenerator(0,0,0,0,1,0,360,5,0.5,200,0.2,1,0.5,0))
    local life = Life(0)
    life.invulnerableTime = 0.5
    ECS.addComponent(e, "Life", life)
    return e
end

function Spawns.spawnEnemy(x, y, speed)
    local e = ECS.createEntity()
    ECS.addComponent(e, "Transform", Transform(x, y, 0))
    ECS.addComponent(e, "Tag", Tag({"Enemy"}))
    ECS.addComponent(e, "Enemy", Enemy(speed))
    ECS.addComponent(e, "Life", Life(config.enemy.life))
    local phys = Physic(1.0, 0.0, true, false)
    phys.vx = -speed
    ECS.addComponent(e, "Physic", phys)
    ECS.addComponent(e, "Collider", Collider("BOX", {1, 1, 1}))
    if hasRendering() then
        ECS.addComponent(e, "Mesh", Mesh("assets/models/cube.obj"))
        ECS.addComponent(e, "Color", Color(1, 0, 0))
    end
    return e
end

function Spawns.createBackground(texturePath)
    if not hasRendering() then return end
    local tex = texturePath or "assets/textures/SinglePlay1.png"
    local bg1 = ECS.createEntity()
    ECS.addComponent(bg1, "Transform", Transform(0, 0, -10, 3.14, 0, 0, -60, 40, 1))
    ECS.addComponent(bg1, "Mesh", Mesh("assets/models/quad.obj", tex))
    ECS.addComponent(bg1, "Background", Background(-2.0, 60.0, -60.0))
    local bg2 = ECS.createEntity()
    ECS.addComponent(bg2, "Transform", Transform(60.0, 0, -10.01, 3.14, 0, 0, 60, 40, 1))
    ECS.addComponent(bg2, "Mesh", Mesh("assets/models/quad.obj", tex))
    ECS.addComponent(bg2, "Background", Background(-2.0, 60.0, -60.0))
end

function Spawns.spawnBullet(x, y, z, isLocal)
    local e = ECS.createEntity()
    ECS.addComponent(e, "Transform", Transform(x, y, z))
    ECS.addComponent(e, "Tag", Tag({"Bullet"}))
    ECS.addComponent(e, "Bullet", Bullet(config.bullet.damage))
    ECS.addComponent(e, "Life", Life(config.bullet.life))

    local phys = Physic(0.1, 0, true, false)
    phys.vx = config.bullet.speed
    ECS.addComponent(e, "Physic", phys)

    ECS.addComponent(e, "Collider", Collider("SPHERE", {0.2}))

    if hasRendering() then
        ECS.addComponent(e, "Mesh", Mesh("assets/models/cube.obj"))
        ECS.addComponent(e, "Color", Color(1, 1, 0)) -- Jaune
        local transform = ECS.getComponent(e, "Transform")
        transform.sx = 0.5
        transform.sy = 0.1
        transform.sz = 0.1
    end

    return e
end

return Spawns