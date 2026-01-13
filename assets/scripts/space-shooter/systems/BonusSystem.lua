-- ============================================================================
-- BonusSystem.lua - Server-Authoritative Bonus Spawning
-- ============================================================================
-- Only spawns bonuses on authoritative instances (Server or Solo).
-- Clients receive bonus entities via NetworkSystem state updates.
-- ============================================================================
local BonusSystem = {}

BonusSystem.spawnTimer = 0
BonusSystem.spawnInterval = 10.0 -- Spawn bonus every 10 seconds

function BonusSystem.init()
    print("[BonusSystem] Initialized (hasAuthority: " .. tostring(ECS.capabilities.hasAuthority) .. ")")
end

function BonusSystem.update(dt)
    if ECS.isPaused then return end

    -- Only run on authoritative instances (Server or Solo)
    if not ECS.capabilities.hasAuthority then return end
    if not ECS.isGameRunning then return end
    
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
    local x = 20 -- Spawn off-screen right

    local bonus = ECS.createEntity()
    ECS.addComponent(bonus, "Transform", Transform(x, y, 0))
    ECS.addComponent(bonus, "Collider", Collider("Box", {0.8, 0.8, 0.8}))
    ECS.addComponent(bonus, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(bonus, "Bonus", Bonus(5.0)) -- 5 second powerup duration
    ECS.addComponent(bonus, "Tag", Tag({"Bonus"}))
    ECS.addComponent(bonus, "Life", Life(1))
    
    -- Network architecture: Server manages bonuses
    if ECS.capabilities.hasAuthority then
        ECS.addComponent(bonus, "ServerAuthority", ServerAuthority())
    end
    
    -- Rendering only on instances with rendering capability
    if ECS.capabilities.hasRendering then
        ECS.addComponent(bonus, "Mesh", Mesh("assets/models/cube.obj"))
        ECS.addComponent(bonus, "Color", Color(0.0, 0.5, 1.0)) -- Blue bonus
    end

    local t = ECS.getComponent(bonus, "Transform")
    t.sx = 0.8
    t.sy = 0.8
    t.sz = 0.8

    local p = ECS.getComponent(bonus, "Physic")
    p.vx = -3.0 -- Move left slower than enemies
end

ECS.registerSystem(BonusSystem)
