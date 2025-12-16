local config = dofile("assets/scripts/space-shooter/config.lua")
local Spawns = {}

-- ============================================================================
-- HELPER: Determine if we have authority/prediction based on global caps
-- ============================================================================
local function hasAuthority()
    return ECS.capabilities and ECS.capabilities.hasAuthority
end

local function hasRendering()
    return ECS.capabilities and ECS.capabilities.hasRendering
end

-- ============================================================================
-- PLAYER
-- ============================================================================
function Spawns.createPlayer(x, y, z, clientId)
    local e = ECS.createEntity()
    print("[Spawns] Creating Player Entity ID: " .. e)

    -- 1. Core Components
    ECS.addComponent(e, "Transform", Transform(x, y, z, 0, 0, 0, 1, 1, 1))
    ECS.addComponent(e, "Tag", Tag({"Player"}))
    -- Physic(mass, friction, fixedRotation, useGravity)
    ECS.addComponent(e, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(e, "Collider", Collider("BOX", {1, 1, 1}))
    ECS.addComponent(e, "Player", Player(config.player.speed))
    ECS.addComponent(e, "Weapon", Weapon(0.2))
    ECS.addComponent(e, "Life", Life(100))
    ECS.addComponent(e, "Score", Score(0))

    -- 2. Input & Logic
    ECS.addComponent(e, "InputState", InputState())

    -- 3. Visuals (Only if rendering is enabled)
    if hasRendering() then
        ECS.addComponent(e, "Mesh", Mesh("assets/models/aircraft.obj"))
        -- Add a light or other visuals?
    end

    -- 4. Network & Architecture
    -- Default to local player if clientId is nil (Solo mode)
    local ownerId = clientId or 0
    local isLocal = (clientId == nil) or (ECS.capabilities.isClientMode and true) or false

    -- In Solo, we are the owner (0). In Multi Client, we receive our ID.
    -- For now, if called from MenuSystem in Solo, clientId is nil.

    ECS.addComponent(e, "NetworkIdentity", NetworkIdentity(e, ownerId, isLocal))

    if hasAuthority() then
        ECS.addComponent(e, "ServerAuthority", ServerAuthority())
    end

    if isLocal or hasRendering() then
        ECS.addComponent(e, "ClientPredicted", ClientPredicted())
        ECS.addComponent(e, "InputBuffer", InputBuffer(60))
    end

    return e
end

-- ============================================================================
-- ENEMY
-- ============================================================================
function Spawns.spawnEnemy(x, y, speed)
    local e = ECS.createEntity()
    
    -- Core
    ECS.addComponent(e, "Transform", Transform(x, y, 0, 0, 0, 0, 1, 1, 1))
    ECS.addComponent(e, "Tag", Tag({"Enemy"}))
    ECS.addComponent(e, "Enemy", Enemy(speed))
    ECS.addComponent(e, "Life", Life(config.enemy.life))
    ECS.addComponent(e, "Score", Score(config.score.kill))

    -- Physics
    local phys = Physic(1.0, 0.0, true, false)
    phys.vx = -speed -- Move left by default
    ECS.addComponent(e, "Physic", phys)
    ECS.addComponent(e, "Collider", Collider(config.enemy.collider.type, config.enemy.collider.size))

    -- Visuals
    if hasRendering() then
        ECS.addComponent(e, "Mesh", Mesh("assets/models/cube.obj"))
        ECS.addComponent(e, "Color", Color(1.0, 0.0, 0.0)) -- Red
    end

    -- Network
    ECS.addComponent(e, "NetworkIdentity", NetworkIdentity(e, -1, false)) -- -1 = Server owned
    if hasAuthority() then
        ECS.addComponent(e, "ServerAuthority", ServerAuthority())
    end

    return e
end

-- ============================================================================
-- BULLET
-- ============================================================================
function Spawns.spawnBullet(x, y, z, isEnemy)
    local e = ECS.createEntity()
    
    local speed = isEnemy and -config.bullet.speed or config.bullet.speed
    local tag = isEnemy and "EnemyBullet" or "Bullet"
    local color = isEnemy and Color(1, 0, 0) or Color(1, 1, 0)

    -- Core
    ECS.addComponent(e, "Transform", Transform(x, y, z, 0, 0, 0, 0.2, 0.2, 0.2))
    ECS.addComponent(e, "Tag", Tag({tag}))
    ECS.addComponent(e, "Bullet", Bullet(config.bullet.damage))
    ECS.addComponent(e, "Life", Life(config.bullet.life))

    -- Physics
    local phys = Physic(0.1, 0.0, true, false)
    phys.vx = speed
    ECS.addComponent(e, "Physic", phys)
    ECS.addComponent(e, "Collider", Collider(config.bullet.collider.type, config.bullet.collider.size))

    -- Visuals
    if hasRendering() then
        ECS.addComponent(e, "Mesh", Mesh("assets/models/cube.obj"))
        ECS.addComponent(e, "Color", color)
    end

    -- Network
    ECS.addComponent(e, "NetworkIdentity", NetworkIdentity(e, -1, false))
    if hasAuthority() then
        ECS.addComponent(e, "ServerAuthority", ServerAuthority())
    end

    return e
end
return Spawns