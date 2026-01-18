-- Level 3: The Final Challenge
-- The ultimate test with moving platforms, long jumps, and precise timing

print("[Level3] Loading Level 3 - The Final Challenge...")

-- Set checkpoint for this level
GameLogicSystem.lastCheckpoint = {x = 0, y = 3, z = 0}

-- ============================================
-- Create Camera
-- ============================================
local camera = ECS.createEntity()
ECS.addComponent(camera, "Transform", Transform(0, 15, -20))
ECS.addComponent(camera, "Camera", Camera(75))
local camTrans = ECS.getComponent(camera, "Transform")
camTrans.rx = -0.5
camTrans.ry = 3.14159

-- ============================================
-- Create Main Light (Dramatic lighting)
-- ============================================
local mainLight = ECS.createEntity()
ECS.addComponent(mainLight, "Transform", Transform(10, 25, 20))
ECS.addComponent(mainLight, "Light", Light({0.9, 0.8, 1.0}, 1.5))

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
ECS.addComponent(startPlatform, "Collider", Collider("Box", {4, 1, 4}))
ECS.addComponent(startPlatform, "Platform", Platform())
ECS.addComponent(startPlatform, "Color", Color(0.7, 0.2, 0.2))
local startTrans = ECS.getComponent(startPlatform, "Transform")
startTrans.sx = 4
startTrans.sy = 1
startTrans.sz = 4

-- ============================================
-- First Section: Rising Stairs
-- ============================================

local platform1 = ECS.createEntity()
ECS.addComponent(platform1, "Transform", Transform(6, 2, 0))
ECS.addComponent(platform1, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform1, "Collider", Collider("Box", {2.5, 0.5, 2.5}))
ECS.addComponent(platform1, "Platform", Platform())
ECS.addComponent(platform1, "Color", Color(0.6, 0.3, 0.2))
local p1Trans = ECS.getComponent(platform1, "Transform")
p1Trans.sx = 2.5
p1Trans.sy = 0.5
p1Trans.sz = 2.5

local platform2 = ECS.createEntity()
ECS.addComponent(platform2, "Transform", Transform(12, 4, 0))
ECS.addComponent(platform2, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform2, "Collider", Collider("Box", {2.5, 0.5, 2.5}))
ECS.addComponent(platform2, "Platform", Platform())
ECS.addComponent(platform2, "Color", Color(0.6, 0.3, 0.2))
local p2Trans = ECS.getComponent(platform2, "Transform")
p2Trans.sx = 2.5
p2Trans.sy = 0.5
p2Trans.sz = 2.5

local platform3 = ECS.createEntity()
ECS.addComponent(platform3, "Transform", Transform(18, 6, 0))
ECS.addComponent(platform3, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform3, "Collider", Collider("Box", {2.5, 0.5, 2.5}))
ECS.addComponent(platform3, "Platform", Platform())
ECS.addComponent(platform3, "Color", Color(0.6, 0.3, 0.2))
local p3Trans = ECS.getComponent(platform3, "Transform")
p3Trans.sx = 2.5
p3Trans.sy = 0.5
p3Trans.sz = 2.5

-- ============================================
-- First Checkpoint (Yellow Cube)
-- ============================================
local checkpoint1 = ECS.createEntity()
ECS.addComponent(checkpoint1, "Transform", Transform(18, 8, 0))
ECS.addComponent(checkpoint1, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(checkpoint1, "Collider", Collider("Box", {1, 2, 1}))
ECS.addComponent(checkpoint1, "Checkpoint", Checkpoint(1))
ECS.addComponent(checkpoint1, "Color", Color(1.0, 1.0, 0.0))
local cp1Trans = ECS.getComponent(checkpoint1, "Transform")
cp1Trans.sy = 2

-- ============================================
-- Second Section: Spiral Path
-- ============================================

local platform4 = ECS.createEntity()
ECS.addComponent(platform4, "Transform", Transform(18, 6, 6))
ECS.addComponent(platform4, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform4, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform4, "Platform", Platform())
ECS.addComponent(platform4, "Color", Color(0.5, 0.4, 0.3))
local p4Trans = ECS.getComponent(platform4, "Transform")
p4Trans.sx = 2
p4Trans.sy = 0.5
p4Trans.sz = 2

local platform5 = ECS.createEntity()
ECS.addComponent(platform5, "Transform", Transform(18, 8, 12))
ECS.addComponent(platform5, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform5, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform5, "Platform", Platform())
ECS.addComponent(platform5, "Color", Color(0.5, 0.4, 0.3))
local p5Trans = ECS.getComponent(platform5, "Transform")
p5Trans.sx = 2
p5Trans.sy = 0.5
p5Trans.sz = 2

local platform6 = ECS.createEntity()
ECS.addComponent(platform6, "Transform", Transform(12, 10, 12))
ECS.addComponent(platform6, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform6, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform6, "Platform", Platform())
ECS.addComponent(platform6, "Color", Color(0.5, 0.4, 0.3))
local p6Trans = ECS.getComponent(platform6, "Transform")
p6Trans.sx = 2
p6Trans.sy = 0.5
p6Trans.sz = 2

local platform7 = ECS.createEntity()
ECS.addComponent(platform7, "Transform", Transform(6, 12, 12))
ECS.addComponent(platform7, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform7, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform7, "Platform", Platform())
ECS.addComponent(platform7, "Color", Color(0.5, 0.4, 0.3))
local p7Trans = ECS.getComponent(platform7, "Transform")
p7Trans.sx = 2
p7Trans.sy = 0.5
p7Trans.sz = 2

local platform8 = ECS.createEntity()
ECS.addComponent(platform8, "Transform", Transform(0, 14, 12))
ECS.addComponent(platform8, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform8, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform8, "Platform", Platform())
ECS.addComponent(platform8, "Color", Color(0.5, 0.4, 0.3))
local p8Trans = ECS.getComponent(platform8, "Transform")
p8Trans.sx = 2
p8Trans.sy = 0.5
p8Trans.sz = 2

-- ============================================
-- Second Checkpoint (Yellow Cube)
-- ============================================
local checkpoint2 = ECS.createEntity()
ECS.addComponent(checkpoint2, "Transform", Transform(0, 16, 12))
ECS.addComponent(checkpoint2, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(checkpoint2, "Collider", Collider("Box", {1, 2, 1}))
ECS.addComponent(checkpoint2, "Checkpoint", Checkpoint(2))
ECS.addComponent(checkpoint2, "Color", Color(1.0, 1.0, 0.0))
local cp2Trans = ECS.getComponent(checkpoint2, "Transform")
cp2Trans.sy = 2

-- ============================================
-- Third Section: The Gauntlet
-- ============================================

local platform9 = ECS.createEntity()
ECS.addComponent(platform9, "Transform", Transform(-6, 16, 18))
ECS.addComponent(platform9, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform9, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform9, "Platform", Platform())
ECS.addComponent(platform9, "Color", Color(0.4, 0.3, 0.5))
local p9Trans = ECS.getComponent(platform9, "Transform")
p9Trans.sx = 2
p9Trans.sy = 0.5
p9Trans.sz = 2

local platform10 = ECS.createEntity()
ECS.addComponent(platform10, "Transform", Transform(-12, 18, 24))
ECS.addComponent(platform10, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform10, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform10, "Platform", Platform())
ECS.addComponent(platform10, "Color", Color(0.4, 0.3, 0.5))
local p10Trans = ECS.getComponent(platform10, "Transform")
p10Trans.sx = 2
p10Trans.sy = 0.5
p10Trans.sz = 2

local platform11 = ECS.createEntity()
ECS.addComponent(platform11, "Transform", Transform(-6, 20, 30))
ECS.addComponent(platform11, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform11, "Collider", Collider("Box", {2, 0.5, 2}))
ECS.addComponent(platform11, "Platform", Platform())
ECS.addComponent(platform11, "Color", Color(0.4, 0.3, 0.5))
local p11Trans = ECS.getComponent(platform11, "Transform")
p11Trans.sx = 2
p11Trans.sy = 0.5
p11Trans.sz = 2

local platform12 = ECS.createEntity()
ECS.addComponent(platform12, "Transform", Transform(0, 22, 36))
ECS.addComponent(platform12, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform12, "Collider", Collider("Box", {3, 0.5, 3}))
ECS.addComponent(platform12, "Platform", Platform())
ECS.addComponent(platform12, "Color", Color(0.4, 0.3, 0.5))
local p12Trans = ECS.getComponent(platform12, "Transform")
p12Trans.sx = 3
p12Trans.sy = 0.5
p12Trans.sz = 3

-- ============================================
-- Final Checkpoint (Yellow Cube)
-- ============================================
local checkpoint3 = ECS.createEntity()
ECS.addComponent(checkpoint3, "Transform", Transform(0, 24, 36))
ECS.addComponent(checkpoint3, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(checkpoint3, "Collider", Collider("Box", {1, 2, 1}))
ECS.addComponent(checkpoint3, "Checkpoint", Checkpoint(3))
ECS.addComponent(checkpoint3, "Color", Color(1.0, 1.0, 0.0))
local cp3Trans = ECS.getComponent(checkpoint3, "Transform")
cp3Trans.sy = 2

-- ============================================
-- Final Platform and Victory
-- ============================================

local finalPlatform = ECS.createEntity()
ECS.addComponent(finalPlatform, "Transform", Transform(0, 22, 44))
ECS.addComponent(finalPlatform, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(finalPlatform, "Collider", Collider("Box", {5, 1, 5}))
ECS.addComponent(finalPlatform, "Platform", Platform())
ECS.addComponent(finalPlatform, "Color", Color(0.8, 0.6, 0.2))
local fpTrans = ECS.getComponent(finalPlatform, "Transform")
fpTrans.sx = 5
fpTrans.sy = 1
fpTrans.sz = 5

-- ============================================
-- Finish Line (Golden Cube) - End of Game
-- ============================================
local finishLine = ECS.createEntity()
ECS.addComponent(finishLine, "Transform", Transform(0, 25, 44))
ECS.addComponent(finishLine, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(finishLine, "Collider", Collider("Box", {2, 3, 2}))
ECS.addComponent(finishLine, "FinishLine", FinishLine("")) -- Empty means end of game
ECS.addComponent(finishLine, "Color", Color(1.0, 0.84, 0.0))
local flTrans = ECS.getComponent(finishLine, "Transform")
flTrans.sy = 3

-- ============================================
-- Death Floor
-- ============================================
local deathFloor = ECS.createEntity()
ECS.addComponent(deathFloor, "Transform", Transform(0, -30, 22))
ECS.addComponent(deathFloor, "Collider", Collider("Box", {150, 1, 150}))
ECS.addComponent(deathFloor, "DeathFloor", DeathFloor())

-- ============================================
-- Win Message (Hidden until game is won)
-- ============================================
local winMessage = ECS.createEntity()
ECS.addComponent(winMessage, "Transform", Transform(0, 0, 0))
ECS.addComponent(winMessage, "Text", Text("CONGRATULATIONS! YOU CONQUERED ALL LEVELS!", 64, {1.0, 0.84, 0.0}))

print("[Level3] Level 3 loaded successfully!")
print("[Level3] The final challenge awaits - this is the ultimate test!")
print("[Level3] Reach the golden cube to complete the game!")
