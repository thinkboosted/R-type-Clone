local BonusSystem = {}

BonusSystem.spawnTimer = 0
BonusSystem.spawnInterval = 10.0 -- Spawn bonus every 10 seconds

function BonusSystem.init()
    print("[BonusSystem] Initialized")
end

function BonusSystem.update(dt)
    BonusSystem.spawnTimer = BonusSystem.spawnTimer + dt
    if BonusSystem.spawnTimer >= BonusSystem.spawnInterval then
        BonusSystem.spawnTimer = 0
        BonusSystem.spawnBonus()
    end

    local bonuses = ECS.getEntitiesWith({"Bonus", "Transform"})
    for _, id in ipairs(bonuses) do
        local t = ECS.getComponent(id, "Transform")
        if t.x < -15 then
            ECS.destroyEntity(id)
        end
    end
end

function BonusSystem.spawnBonus()
    local y = math.random(-5, 5)
    local x = 15 -- Spawn off-screen right

    local bonus = ECS.createEntity()
    ECS.addComponent(bonus, "Transform", Transform(x, y, 0))
    ECS.addComponent(bonus, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(bonus, "Collider", Collider("Box", {0.8, 0.8, 0.8}))
    ECS.addComponent(bonus, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(bonus, "Bonus", Bonus(5.0)) -- 5 second powerup duration
    ECS.addComponent(bonus, "Life", Life(1))
    ECS.addComponent(bonus, "Color", Color(0.0, 0.5, 1.0)) -- Blue bonus

    local t = ECS.getComponent(bonus, "Transform")
    t.sx = 0.8
    t.sy = 0.8
    t.sz = 0.8

    local p = ECS.getComponent(bonus, "Physic")
    p.vx = -3.0 -- Move left slower than enemies
end

ECS.registerSystem(BonusSystem)
