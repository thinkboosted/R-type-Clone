-- Game Definition

-- Load Components
dofile("assets/scripts/test-minigame/Components.lua")


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
local floorTrans = ECS.getComponent(floor, "Transform")
floorTrans.sx = 50
floorTrans.sy = 0.1
floorTrans.sz = 50

local player = ECS.createEntity()
ECS.addComponent(player, "Transform", Transform(0, 2, 0))
ECS.addComponent(player, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(player, "Collider", Collider("Box", {1, 1, 1}))
ECS.addComponent(player, "Physic", Physic(4.0, 10.0, true))
ECS.addComponent(player, "Player", Player(15.0))

local winCube = ECS.createEntity()
ECS.addComponent(winCube, "Transform", Transform(5, 2, 5))
ECS.addComponent(winCube, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(winCube, "Collider", Collider("Box", {1, 1, 1}))
ECS.addComponent(winCube, "WinCube", WinCube())
ECS.addComponent(winCube, "Physic", Physic(4.0, 10.0, true))
ECS.addComponent(winCube, "Color", Color(1.0, 0.0, 0.0))


-- -- 4. Create Random Boxes
-- for i = 1, 1000 do
--     local box = ECS.createEntity()
--     local x = math.random(-10, 10)
--     local z = math.random(-10, 10)
--     ECS.addComponent(box, "Transform", Transform(x, 5 + i * 2, z))
--     ECS.addComponent(box, "Mesh", Mesh("assets/models/cube.obj"))
--     ECS.addComponent(box, "Collider", Collider("Box", {1, 1, 1}))
--     ECS.addComponent(box, "Physic", Physic(1.0))
-- end

