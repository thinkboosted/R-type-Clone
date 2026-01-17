-- ========================================================
-- BREAKOUT 3D - DEMO GAME
-- ========================================================

print("[Breakout] Initializing...")

-- 1. DEFINE COMPONENTS
-- ========================================================

function Transform(x, y, z, rx, ry, rz, sx, sy, sz)
    return { x = x or 0, y = y or 0, z = z or 0, rx = rx or 0, ry = ry or 0, rz = rz or 0, sx = sx or 1, sy = sy or 1, sz = sz or 1 }
end

function Mesh(model, texture)
    return { modelPath = model, texturePath = texture }
end

function Physic(vx, vy, vz)
    return { vx = vx or 0, vy = vy or 0, vz = vz or 0 }
end

function Collider(type, w, h, d, mass, friction)
    return { type = type, width = w or 1, height = h or 1, depth = d or 1, mass = mass or 1.0, friction = friction or 0.0 }
end

function Paddle(speed)
    return { speed = speed or 20.0 }
end

function Ball(speed)
    return { speed = speed or 15.0, active = false }
end

function Brick(score)
    return { score = score or 100 }
end

function Camera(fov)
    return { fov = fov or 90, isActive = true }
end

-- ========================================================
-- 2. SETUP GAME STATE
-- ========================================================

local Game = {
    state = "MENU", -- MENU, PLAYING, GAMEOVER
    score = 0,
    lives = 3,
    level = 1
}

local ECS = ECS or {}

-- ========================================================
-- 3. LOAD SYSTEMS
-- ========================================================

dofile("assets/scripts/breakout/systems/InputSystem.lua")
dofile("assets/scripts/breakout/systems/PhysicsSystem.lua")
dofile("assets/scripts/breakout/systems/GameLogicSystem.lua")
dofile("assets/scripts/breakout/systems/RenderSystem.lua")

-- ========================================================
-- 4. INIT
-- ========================================================

function Game.init()
    print("[Breakout] Starting Game Init")
    
    -- Setup Camera
    ECS.createEntity({
        Transform = Transform(0, -30, 40, 0.6, 0, 0),
        Camera = Camera(60)
    })
    
    -- Setup Paddle
    -- Note: Collider is now automatically created by the engine when added
    local paddle = ECS.createEntity({
        Transform = Transform(0, -20, 0, 0, 0, 0, 4, 1, 1),
        Mesh = Mesh("assets/models/cube.obj", "assets/textures/plane_texture.png"),
        Paddle = Paddle(30.0),
        Collider = Collider("BOX", 4, 1, 1, 1.0, 0.0), -- Mass 1.0, Friction 0.0
        Physic = Physic(0, 0, 0)
    })
    
    -- Lock rotations and Z movement to keep paddle on plane
    if ECS.setConstraints then
        ECS.setConstraints(paddle, false, false, true, true, true, true)
    end
    
    -- Setup Ball
    local ball = ECS.createEntity({
        Transform = Transform(0, -18, 0, 0, 0, 0, 1, 1, 1),
        Mesh = Mesh("assets/models/sphere.obj", "assets/textures/shoot.jpg"),
        Ball = Ball(20.0),
        Collider = Collider("SPHERE", 1, 1, 1, 1.0, 0.0), -- Mass 1.0, Friction 0.0
        Physic = Physic(0, 0, 0)
    })
    
    if ECS.setConstraints then
        ECS.setConstraints(ball, false, false, true, true, true, true)
    end
    
    -- Setup Bricks
    Game.createBricks()
    
    -- Setup Walls (Invisible colliders)
    Game.createWalls()
    
    print("[Breakout] Init Complete")
end

function Game.createBricks()
    local rows = 5
    local cols = 10
    local startX = -20
    local startY = 10
    local width = 4
    local height = 1.5
    local gap = 0.5
    
    for r = 0, rows - 1 do
        for c = 0, cols - 1 do
            local x = startX + c * (width + gap)
            local y = startY + r * (height + gap)
            
            ECS.createEntity({
                Transform = Transform(x, y, 0, 0, 0, 0, width, height, 1),
                Mesh = Mesh("assets/models/cube.obj", "assets/textures/test.png"),
                Brick = Brick(100),
                Collider = Collider("BOX", width, height, 1, 0.0, 0.0) -- Mass 0.0 (Static)
            })
        end
    end
end

function Game.createWalls()
    -- Left Wall
    ECS.createEntity({
        Transform = Transform(-25, 0, 0, 0, 0, 0, 1, 60, 1),
        Collider = Collider("BOX", 1, 60, 1, 0.0, 0.0) -- Static
    })
    
    -- Right Wall
    ECS.createEntity({
        Transform = Transform(25, 0, 0, 0, 0, 0, 1, 60, 1),
        Collider = Collider("BOX", 1, 60, 1, 0.0, 0.0) -- Static
    })
    
    -- Top Wall
    ECS.createEntity({
        Transform = Transform(0, 25, 0, 0, 0, 0, 50, 1, 1),
        Collider = Collider("BOX", 50, 1, 1, 0.0, 0.0) -- Static
    })
end

ECS.registerSystem(Game)