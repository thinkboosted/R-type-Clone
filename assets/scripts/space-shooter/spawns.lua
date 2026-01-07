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
    print("[Spawns] Adding Transform...")
    ECS.addComponent(e, "Transform", Transform(x, y, z, 0, 0, 0, 1, 1, 1))
    print("[Spawns] Adding Tag...")
    ECS.addComponent(e, "Tag", Tag({"Player"}))
    -- Physic(mass, friction, fixedRotation, useGravity)
    print("[Spawns] Adding Physic...")
    ECS.addComponent(e, "Physic", Physic(1.0, 0.0, true, false))
    print("[Spawns] Adding Collider...")
    ECS.addComponent(e, "Collider", Collider("BOX", {1, 1, 1}))
    print("[Spawns] Adding Player...")
    ECS.addComponent(e, "Player", Player(config.player.speed))
    print("[Spawns] Adding Weapon...")
    ECS.addComponent(e, "Weapon", Weapon(0.2))
    print("[Spawns] Adding Life...")
    ECS.addComponent(e, "Life", Life(100))
    print("[Spawns] Adding Score...")
    ECS.addComponent(e, "Score", Score(0))

    -- 2. Input & Logic
    print("[Spawns] Adding InputState...")
    ECS.addComponent(e, "InputState", InputState())

    -- 3. Visuals (Only if rendering is enabled)
    if hasRendering() then
        print("[Spawns] Adding Mesh...")
        ECS.addComponent(e, "Mesh", Mesh("assets/models/simple_plane.obj", "assets/textures/plane_texture.png"))
        print("[Spawns] Adding Color...")
        ECS.addComponent(e, "Color", Color(1.0, 1.0, 1.0)) -- White (removes default orange)
    end

    -- 4. Network & Architecture
    -- Default to local player if clientId is nil (Solo mode)
    local ownerId = clientId or 0
    local isLocal = (clientId == nil) or (ECS.capabilities.isClientMode and true) or false

    -- In Solo, we are the owner (0). In Multi Client, we receive our ID.
    -- For now, if called from MenuSystem in Solo, clientId is nil.

    print("[Spawns] Adding NetworkIdentity...")
    ECS.addComponent(e, "NetworkIdentity", NetworkIdentity(e, ownerId, isLocal))

    if hasAuthority() then
        print("[Spawns] Adding ServerAuthority...")
        ECS.addComponent(e, "ServerAuthority", ServerAuthority())
    end

    if isLocal or hasRendering() then
        print("[Spawns] Adding ClientPredicted...")
        ECS.addComponent(e, "ClientPredicted", ClientPredicted())
        print("[Spawns] Adding InputBuffer...")
        ECS.addComponent(e, "InputBuffer", InputBuffer(60))
        
        -- Reactor Particles (Blue Trail)
        -- ParticleGenerator(offsetX, offsetY, offsetZ, dirX, dirY, dirZ, spread, speed, lifeTime, rate, size, r, g, b)
        print("[Spawns] Adding ParticleGenerator...")
        ECS.addComponent(e, "ParticleGenerator", ParticleGenerator(
            -1.0, 0, 0,   -- Offset (Behind)
            -1, 0, 0,     -- Direction (Backwards)
            0.2,          -- Spread
            2.0,          -- Speed
            0.5,          -- LifeTime
            50.0,         -- Rate
            0.1,          -- Size
            0.0, 0.5, 1.0 -- Color (Blue)
        ))
    end

    print("[Spawns] Player creation complete!")
    return e
end

-- ============================================================================
-- EXPLOSION
-- ============================================================================
function Spawns.createExplosion(x, y, z)
    if not hasRendering() then return end

    local e = ECS.createEntity()
    
    ECS.addComponent(e, "Transform", Transform(x, y, z))
    ECS.addComponent(e, "ParticleGenerator", ParticleGenerator(
        0, 0, 0,      -- Offset
        0, 1, 0,      -- Direction (Omni-ish due to spread)
        360.0,        -- Spread (Full sphere)
        5.0,          -- Speed
        0.5,          -- LifeTime
        200.0,        -- Rate (Burst)
        0.2,          -- Size
        1.0, 0.5, 0.0 -- Color (Orange)
    ))
    
    -- Self-destruct after 0.5s using LifeSystem logic
    local life = Life(0)
    life.invulnerableTime = 0.5
    ECS.addComponent(e, "Life", life)
    
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

-- ============================================================================
-- BACKGROUND
-- ============================================================================
function Spawns.createBackground(texturePath)
    if not hasRendering() then return end

    local tex = texturePath or "assets/textures/PlutoImage_TutorialLevel.png"
    print("[Spawns] Creating Parallax Background with texture: " .. tex)

    -- Layer 1 (Far Stars)
    local bg1 = ECS.createEntity()
    ECS.addComponent(bg1, "Transform", Transform(0, 0, -10, 3.14, 0, 0, -60, 40, 1))
    ECS.addComponent(bg1, "Mesh", Mesh("assets/models/quad.obj", tex))
    ECS.addComponent(bg1, "Color", Color(1.0, 1.0, 1.0))
    ECS.addComponent(bg1, "Background", Background(-2.0, 60.0, -60.0))

    local bg2 = ECS.createEntity()
    ECS.addComponent(bg2, "Transform", Transform(60.0, 0, -10.01, 3.14, 0, 0, 60, 40, 1))
    ECS.addComponent(bg2, "Mesh", Mesh("assets/models/quad.obj", tex))
    ECS.addComponent(bg2, "Color", Color(1.0, 1.0, 1.0))
    ECS.addComponent(bg2, "Background", Background(-2.0, 60.0, -60.0))

    print("[Spawns] Background entities created: " .. bg1 .. ", " .. bg2)
end

return Spawns