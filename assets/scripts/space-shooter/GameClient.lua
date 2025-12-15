-- Client bootstrap

-- Assumes Components.lua and NetworkSystem.lua already loaded by Game.lua

print("Loading Client Systems...")
dofile("assets/scripts/space-shooter/systems/RenderSystem.lua")
dofile("assets/scripts/space-shooter/systems/InputSystem.lua")
dofile("assets/scripts/space-shooter/systems/MenuSystem.lua")
dofile("assets/scripts/space-shooter/systems/ParticleSystem.lua")
dofile("assets/scripts/space-shooter/systems/ScoreSystem.lua")

-- Gameplay systems stay inert unless ECS.isLocal (solo). Keeps solo working.
dofile("assets/scripts/space-shooter/systems/PhysicSystem.lua")
dofile("assets/scripts/space-shooter/systems/CollisionSystem.lua")
dofile("assets/scripts/space-shooter/systems/LifeSystem.lua")
dofile("assets/scripts/space-shooter/systems/EnemySystem.lua")

-- Setup Camera
local camera = ECS.createEntity()
ECS.addComponent(camera, "Transform", Transform(0, 0, 20))
ECS.addComponent(camera, "Camera", Camera(90))
