-- Game Definition

-- Load Components
dofile("assets/scripts/ecs/Components.lua")

local camera = ECS.createEntity()
ECS.addComponent(camera, "Transform", Transform(0, 5, -10))
ECS.addComponent(camera, "Camera", Camera(90))

local camTrans = ECS.getComponent(camera, "Transform")
camTrans.rx = -0.5
camTrans.ry = 3.14159

local floor = ECS.createEntity()
ECS.addComponent(floor, "Transform", Transform(0, -2, 0))
ECS.addComponent(floor, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(floor, "Collider", Collider("Box", {50, 0.5, 50}))
ECS.addComponent(floor, "Physic", Physic(0.0))
local floorTrans = ECS.getComponent(floor, "Transform")
floorTrans.sx = 50
floorTrans.sy = 0.1
floorTrans.sz = 50

local player = ECS.createEntity()
ECS.addComponent(player, "Transform", Transform(0, 2, 0))
ECS.addComponent(player, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(player, "Collider", Collider("Box", {1, 1, 1}))
ECS.addComponent(player, "Physic", Physic(1.0))
ECS.addComponent(player, "Player", Player(5.0))

print("[Game] Scene Created")
