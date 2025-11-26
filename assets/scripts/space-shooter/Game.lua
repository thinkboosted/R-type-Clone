-- Game Definition

-- Load Components
dofile("assets/scripts/space-shooter/components/Components.lua")

-- Load Systems
dofile("assets/scripts/space-shooter/systems/PhysicSystem.lua")
dofile("assets/scripts/space-shooter/systems/RenderSystem.lua")
dofile("assets/scripts/space-shooter/systems/InputSystem.lua")
dofile("assets/scripts/space-shooter/systems/CollisionSystem.lua")
dofile("assets/scripts/space-shooter/systems/LifeSystem.lua")
dofile("assets/scripts/space-shooter/systems/EnemySystem.lua")
dofile("assets/scripts/space-shooter/systems/BonusSystem.lua")
dofile("assets/scripts/space-shooter/systems/ScoreSystem.lua")
dofile("assets/scripts/space-shooter/systems/FollowSystem.lua")

ECS.loadLastSave("space-shooter-save")

-- Create Camera
local camera = ECS.createEntity()
ECS.addComponent(camera, "Transform", Transform(0, 0, 20))
ECS.addComponent(camera, "Camera", Camera(90))

-- Create Player
local player = ECS.createEntity()
ECS.addComponent(player, "Transform", Transform(-8, 0, 0))
ECS.addComponent(player, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(player, "Collider", Collider("Box", {1, 1, 1}))
ECS.addComponent(player, "Physic", Physic(1.0, 0.0, true, false)) -- mass 1, friction 0, fixedRot, noGravity
ECS.addComponent(player, "Player", Player(20.0))
ECS.addComponent(player, "Life", Life(3))
ECS.addComponent(player, "Color", Color(0.0, 1.0, 0.0)) -- Green player

-- Create Score Entity
local scoreEntity = ECS.createEntity()
ECS.addComponent(scoreEntity, "Score", Score(0))
ECS.addComponent(scoreEntity, "Transform", Transform(650, 550, 0))
ECS.addComponent(scoreEntity, "Text", Text("Score: 0", "assets/fonts/arial.ttf", 24, true))
ECS.addComponent(scoreEntity, "Color", Color(1.0, 1.0, 1.0))

-- Create Player Tag
local playerTag = ECS.createEntity()
ECS.addComponent(playerTag, "Transform", Transform(0, 0, 0))
ECS.addComponent(playerTag, "Text", Text("Player1", "assets/fonts/arial.ttf", 24, false))
ECS.addComponent(playerTag, "Color", Color(1.0, 1.0, 0.0))
ECS.addComponent(playerTag, "Follow", Follow(player, 0, 1.5, 0))

print("Space Shooter Game Loaded")
