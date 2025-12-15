-- Server bootstrap

-- Assumes Components.lua and NetworkSystem.lua already loaded by Game.lua

print("Loading Server Systems...")
dofile("assets/scripts/space-shooter/systems/PhysicSystem.lua")
dofile("assets/scripts/space-shooter/systems/CollisionSystem.lua")
dofile("assets/scripts/space-shooter/systems/LifeSystem.lua")
dofile("assets/scripts/space-shooter/systems/EnemySystem.lua")
dofile("assets/scripts/space-shooter/systems/InputSystem.lua")

-- Server always runs the simulation
ecsServerInitialized = true
ECS.isGameRunning = true
