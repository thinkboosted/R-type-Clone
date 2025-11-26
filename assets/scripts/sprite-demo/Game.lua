-- Game Definition

-- Load Components
dofile("assets/scripts/sprite-demo/Components.lua")
dofile("assets/scripts/sprite-demo/RenderSystem.lua")

-- Camera
local camera = ECS.createEntity()
ECS.addComponent(camera, "Transform", Transform(0, 5, -10))
ECS.addComponent(camera, "Camera", Camera(90))
local camTrans = ECS.getComponent(camera, "Transform")
camTrans.rx = -0.5
camTrans.ry = 3.14159

-- 3D Mesh (Floor)
local floor = ECS.createEntity()
ECS.addComponent(floor, "Transform", Transform(0, -2, 0))
ECS.addComponent(floor, "Mesh", Mesh("assets/models/cube.obj"))
local floorTrans = ECS.getComponent(floor, "Transform")
floorTrans.sx = 10
floorTrans.sy = 0.1
floorTrans.sz = 10
ECS.addComponent(floor, "Color", Color(0.5, 0.5, 0.5))

-- 3D Sprite (World Space)
local sprite3D = ECS.createEntity()
ECS.addComponent(sprite3D, "Transform", Transform(0, 1, 0))
ECS.addComponent(sprite3D, "Sprite", Sprite("assets/textures/character.png", false))
local s3dTrans = ECS.getComponent(sprite3D, "Transform")
s3dTrans.sx = 2
s3dTrans.sy = 2

-- 2D Sprite (Screen Space / HUD)
local sprite2D = ECS.createEntity()
ECS.addComponent(sprite2D, "Transform", Transform(50, 50, 0)) -- 50, 50 pixels from bottom-left
ECS.addComponent(sprite2D, "Sprite", Sprite("assets/textures/hud_icon.png", true))
local s2dTrans = ECS.getComponent(sprite2D, "Transform")
s2dTrans.sx = 0.5 -- Scale relative to texture size
s2dTrans.sy = 0.5

print("Game Loaded")
