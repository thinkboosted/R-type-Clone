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
    local s = config.player.scale or 0.7
    ECS.addComponent(e, "Transform", Transform(x, y, z, 0, 0, 0, s, s, s))
    ECS.addComponent(e, "Tag", Tag({"Player"}))
    -- Physic(mass, friction, fixedRotation, useGravity)
    ECS.addComponent(e, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(e, "Collider", Collider("BOX", {s, s, s}))
    ECS.addComponent(e, "Player", Player(config.player.speed))
    ECS.addComponent(e, "Weapon", Weapon(0.2))
    ECS.addComponent(e, "Life", Life(100))
    ECS.addComponent(e, "Score", Score(0))

    -- 2. Input & Logic
    ECS.addComponent(e, "InputState", InputState())

    -- 3. Visuals (Only if rendering is enabled)
    if hasRendering() then
        ECS.addComponent(e, "Mesh", Mesh("assets/models/simple_plane.obj", "assets/textures/plane_texture.png"))
        ECS.addComponent(e, "Color", Color(1.0, 1.0, 1.0)) -- White (removes default orange)
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
        
        -- Reactor Particles (Blue Trail)
        -- ParticleGenerator(offsetX, offsetY, offsetZ, dirX, dirY, dirZ, spread, speed, lifeTime, rate, size, r, g, b)
        ECS.addComponent(e, "ParticleGenerator", ParticleGenerator(
            -1.0 * s, 0, 0,   -- Offset (Behind)
            -1, 0, 0,     -- Direction (Backwards)
            0.2,          -- Spread
            2.0,          -- Speed
            0.5,          -- LifeTime
            50.0,         -- Rate
            0.1,          -- Size
            0.0, 0.5, 1.0 -- Color (Blue)
        ))
    end

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
function Spawns.spawnEnemy(x, y, speed, type)
    local e = ECS.createEntity()
    
    -- Determine Monster Type (1, 2, or 3)
    local mType = type or math.random(1, 3)
    
    -- Configuration per Type
    local configType = {
        [1] = {
            name = "Monster_1",
            frames = 8,
            pattern = "sine",
            amp = 1.0, freq = 1.0 -- Big smooth wave
        },
        [2] = {
            name = "Monster_2",
            frames = 3,
            pattern = "zigzag",
            amp = 1.0, freq = 0.3 -- Sharp mechanical movement
        },
        [3] = {
            name = "Monster_3",
            frames = 8,
            pattern = "circle",
            amp = 1.0, freq = 1.0 -- Looping flight
        }
    }
    
    local cfg = configType[mType] or configType[1]

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
    
    -- Movement Pattern (Linked to Type)
    ECS.addComponent(e, "MovementPattern", MovementPattern(cfg.pattern, cfg.amp, cfg.freq, speed))

    -- Visuals
    if hasRendering() then
        local basePath = "assets/models/" .. cfg.name .. "/motion_"
        local texturePath = "assets/textures/Ennemies/" .. cfg.name .. "/motion_"
        
        -- Start with Frame 1
        ECS.addComponent(e, "Mesh", Mesh(basePath .. "1.obj", texturePath .. "1.png"))
        ECS.addComponent(e, "Color", Color(1.0, 1.0, 1.0))
        
        -- Animate
        ECS.addComponent(e, "Animation", Animation(
            cfg.frames,    -- per type
            0.1,  -- 10 FPS
            true, -- Loop
            basePath,
            texturePath
        ))
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
        if isEnemy then
             ECS.addComponent(e, "Mesh", Mesh("assets/models/sphere.obj", nil)) -- Orb
             ECS.addComponent(e, "Color", Color(1.0, 0.5, 0.0)) -- Orange
        else
             ECS.addComponent(e, "Mesh", Mesh("assets/models/laser.obj", nil)) -- Laser
             ECS.addComponent(e, "Color", Color(0.0, 1.0, 1.0)) -- Cyan
        end
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