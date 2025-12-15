-- Game Definition

-- Load Components
dofile("assets/scripts/space-shooter/components/Components.lua")

-- Load Systems Common to both (or specifically managed inside)
dofile("assets/scripts/space-shooter/systems/NetworkSystem.lua")

if ECS.isServer() then
    print("Loading Server Systems...")
    dofile("assets/scripts/space-shooter/systems/PhysicSystem.lua")
    dofile("assets/scripts/space-shooter/systems/CollisionSystem.lua")
    dofile("assets/scripts/space-shooter/systems/LifeSystem.lua")
    dofile("assets/scripts/space-shooter/systems/EnemySystem.lua")
    dofile("assets/scripts/space-shooter/systems/InputSystem.lua") -- Server handles physics from inputs
else
    print("Loading Client Systems...")
    dofile("assets/scripts/space-shooter/systems/RenderSystem.lua")
    dofile("assets/scripts/space-shooter/systems/InputSystem.lua") -- Client sends inputs
    dofile("assets/scripts/space-shooter/systems/MenuSystem.lua")  -- Menu UI
    dofile("assets/scripts/space-shooter/systems/ParticleSystem.lua")
    dofile("assets/scripts/space-shooter/systems/ScoreSystem.lua")
end

-- Setup Client Scene (Camera & UI)
if not ECS.isServer() then
    -- Create Camera
    local camera = ECS.createEntity()
    ECS.addComponent(camera, "Transform", Transform(0, 0, 20))
    ECS.addComponent(camera, "Camera", Camera(90))
end

print("Space Shooter Game Loaded")
