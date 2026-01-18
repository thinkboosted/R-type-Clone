-- Level 2: Stepping Up
-- A more challenging level with more platforms and jumps

print("[Level2] Loading Level 2...")

-- Set checkpoint for this level
GameLogicSystem.lastCheckpoint = {x = 0, y = 3, z = 0}

-- ============================================
-- Create Camera
-- ============================================
local camera = ECS.createEntity()
ECS.addComponent(camera, "Transform", Transform(0, 10, -15))
ECS.addComponent(camera, "Camera", Camera(75))
local camTrans = ECS.getComponent(camera, "Transform")
camTrans.rx = -0.4
camTrans.ry = 3.14159

-- ============================================
-- Create Main Light (Warm sunlight)
-- ============================================
local mainLight = ECS.createEntity()
ECS.addComponent(mainLight, "Transform", Transform(5, 20, 15))
ECS.addComponent(mainLight, "Light", Light({1.0, 0.9, 0.8}, 1.3))

-- ============================================
-- Create Player (Blue Cube)
-- ============================================
local player = ECS.createEntity()
ECS.addComponent(player, "Transform", Transform(0, 3, 0))
ECS.addComponent(player, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(player, "Collider", Collider("Box", {1, 1, 1}))
ECS.addComponent(player, "Physic", Physic(15.0, 4.0, true))
ECS.addComponent(player, "Player", Player(12.0, 100.0))
ECS.addComponent(player, "Color", Color(0.2, 0.4, 0.9))

-- ============================================
-- Create Starting Platform
-- ============================================
local startPlatform = ECS.createEntity()
ECS.addComponent(startPlatform, "Transform", Transform(0, 0, 0))
ECS.addComponent(startPlatform, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(startPlatform, "Collider", Collider("Box", {5, 1, 5}))
ECS.addComponent(startPlatform, "Platform", Platform())
ECS.addComponent(startPlatform, "Color", Color(0.6, 0.3, 0.3))
local startTrans = ECS.getComponent(startPlatform, "Transform")
startTrans.sx = 5
startTrans.sy = 1
startTrans.sz = 5

-- ============================================
-- Create Zigzag Platform Path
-- ============================================

-- Platform series going right
local platform1 = ECS.createEntity()
ECS.addComponent(platform1, "Transform", Transform(8, 2, 3))
ECS.addComponent(platform1, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform1, "Collider", Collider("Box", {3, 0.5, 3}))
ECS.addComponent(platform1, "Platform", Platform())
ECS.addComponent(platform1, "Color", Color(0.5, 0.3, 0.3))
local p1Trans = ECS.getComponent(platform1, "Transform")
p1Trans.sx = 3
p1Trans.sy = 0.5
p1Trans.sz = 3

local platform2 = ECS.createEntity()
ECS.addComponent(platform2, "Transform", Transform(16, 4, 0))
ECS.addComponent(platform2, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform2, "Collider", Collider("Box", {3, 0.5, 3}))
ECS.addComponent(platform2, "Platform", Platform())
ECS.addComponent(platform2, "Color", Color(0.5, 0.3, 0.3))
local p2Trans = ECS.getComponent(platform2, "Transform")
p2Trans.sx = 3
p2Trans.sy = 0.5
p2Trans.sz = 3

local platform3 = ECS.createEntity()
ECS.addComponent(platform3, "Transform", Transform(16, 6, 8))
ECS.addComponent(platform3, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform3, "Collider", Collider("Box", {3, 0.5, 3}))
ECS.addComponent(platform3, "Platform", Platform())
ECS.addComponent(platform3, "Color", Color(0.5, 0.3, 0.3))
local p3Trans = ECS.getComponent(platform3, "Transform")
p3Trans.sx = 3
p3Trans.sy = 0.5
p3Trans.sz = 3

-- ============================================
-- First Checkpoint (Yellow Cube)
-- ============================================
local checkpoint1 = ECS.createEntity()
ECS.addComponent(checkpoint1, "Transform", Transform(16, 8, 8))
ECS.addComponent(checkpoint1, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(checkpoint1, "Collider", Collider("Box", {1, 2, 1}))
ECS.addComponent(checkpoint1, "Checkpoint", Checkpoint(1))
ECS.addComponent(checkpoint1, "Color", Color(1.0, 1.0, 0.0))
local cp1Trans = ECS.getComponent(checkpoint1, "Transform")
cp1Trans.sy = 2

-- Continue platforms zigzag back
local platform4 = ECS.createEntity()
ECS.addComponent(platform4, "Transform", Transform(8, 8, 14))
ECS.addComponent(platform4, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform4, "Collider", Collider("Box", {3, 0.5, 3}))
ECS.addComponent(platform4, "Platform", Platform())
ECS.addComponent(platform4, "Color", Color(0.4, 0.3, 0.4))
local p4Trans = ECS.getComponent(platform4, "Transform")
p4Trans.sx = 3
p4Trans.sy = 0.5
p4Trans.sz = 3

local platform5 = ECS.createEntity()
ECS.addComponent(platform5, "Transform", Transform(0, 10, 20))
ECS.addComponent(platform5, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform5, "Collider", Collider("Box", {3, 0.5, 3}))
ECS.addComponent(platform5, "Platform", Platform())
ECS.addComponent(platform5, "Color", Color(0.4, 0.3, 0.4))
local p5Trans = ECS.getComponent(platform5, "Transform")
p5Trans.sx = 3
p5Trans.sy = 0.5
p5Trans.sz = 3

local platform6 = ECS.createEntity()
ECS.addComponent(platform6, "Transform", Transform(-8, 12, 26))
ECS.addComponent(platform6, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform6, "Collider", Collider("Box", {3, 0.5, 3}))
ECS.addComponent(platform6, "Platform", Platform())
ECS.addComponent(platform6, "Color", Color(0.4, 0.3, 0.4))
local p6Trans = ECS.getComponent(platform6, "Transform")
p6Trans.sx = 3
p6Trans.sy = 0.5
p6Trans.sz = 3

-- ============================================
-- Second Checkpoint (Yellow Cube)
-- ============================================
local checkpoint2 = ECS.createEntity()
ECS.addComponent(checkpoint2, "Transform", Transform(-8, 14, 26))
ECS.addComponent(checkpoint2, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(checkpoint2, "Collider", Collider("Box", {1, 2, 1}))
ECS.addComponent(checkpoint2, "Checkpoint", Checkpoint(2))
ECS.addComponent(checkpoint2, "Color", Color(1.0, 1.0, 0.0))
local cp2Trans = ECS.getComponent(checkpoint2, "Transform")
cp2Trans.sy = 2

-- Final stretch - narrower platforms
local platform7 = ECS.createEntity()
ECS.addComponent(platform7, "Transform", Transform(-16, 14, 32))
ECS.addComponent(platform7, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform7, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform7, "Platform", Platform())
ECS.addComponent(platform7, "Color", Color(0.3, 0.3, 0.5))
local p7Trans = ECS.getComponent(platform7, "Transform")
p7Trans.sx = 2
p7Trans.sy = 0.5
p7Trans.sz = 2

local platform8 = ECS.createEntity()
ECS.addComponent(platform8, "Transform", Transform(-16, 16, 38))
ECS.addComponent(platform8, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform8, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform8, "Platform", Platform())
ECS.addComponent(platform8, "Color", Color(0.3, 0.3, 0.5))
local p8Trans = ECS.getComponent(platform8, "Transform")
p8Trans.sx = 2
p8Trans.sy = 0.5
p8Trans.sz = 2

local platform9 = ECS.createEntity()
ECS.addComponent(platform9, "Transform", Transform(-8, 18, 44))
ECS.addComponent(platform9, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform9, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform9, "Platform", Platform())
ECS.addComponent(platform9, "Color", Color(0.3, 0.3, 0.5))
local p9Trans = ECS.getComponent(platform9, "Transform")
p9Trans.sx = 2
p9Trans.sy = 0.5
p9Trans.sz = 2

-- ============================================
-- Finish Platform
-- ============================================
local finishPlatform = ECS.createEntity()
ECS.addComponent(finishPlatform, "Transform", Transform(0, 20, 50))
ECS.addComponent(finishPlatform, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(finishPlatform, "Collider", Collider("Box", {6, 1, 6}))
ECS.addComponent(finishPlatform, "Platform", Platform())
ECS.addComponent(finishPlatform, "Color", Color(0.3, 0.7, 0.3))
local fpTrans = ECS.getComponent(finishPlatform, "Transform")
fpTrans.sx = 6
fpTrans.sy = 1
fpTrans.sz = 6

-- ============================================
-- Finish Line (Green Cube) - Leads to Level 3
-- ============================================
local finishLine = ECS.createEntity()
ECS.addComponent(finishLine, "Transform", Transform(0, 16, 36))
ECS.addComponent(finishLine, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(finishLine, "Collider", Collider("Box", {2, 3, 2}))
ECS.addComponent(finishLine, "FinishLine", FinishLine("assets/scripts/platformer/levels/Level3.lua"))
ECS.addComponent(finishLine, "Color", Color(0.0, 1.0, 0.0))
local flTrans = ECS.getComponent(finishLine, "Transform")
flTrans.sy = 3

-- ============================================
-- Death Floor
-- ============================================
local deathFloor = ECS.createEntity()
ECS.addComponent(deathFloor, "Transform", Transform(0, -20, 18))
ECS.addComponent(deathFloor, "Collider", Collider("Box", {100, 1, 100}))
ECS.addComponent(deathFloor, "DeathFloor", DeathFloor())

-- ============================================
-- Win Message (Hidden until game is won)
-- ============================================
-- Note: Win message moved to Level 3 as final level

print("[Level2] Level 2 loaded successfully!")
print("[Level2] This level is more challenging - watch your jumps!")
print("[Level2] Reach the green cube to continue to Level 3!")
