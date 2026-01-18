-- Level 1: Introduction
-- A simple level to get the player familiar with the controls

print("[Level1] Loading Level 1...")

-- Clear previous level entities (if any)
-- Note: In a full implementation, you'd want to clean up old entities

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
-- Create Main Light
-- ============================================
local mainLight = ECS.createEntity()
ECS.addComponent(mainLight, "Transform", Transform(0, 15, 10))
ECS.addComponent(mainLight, "Light", Light({1.0, 0.95, 0.9}, 1.2))

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
ECS.addComponent(startPlatform, "Collider", Collider("Box", {6, 1, 6}))
ECS.addComponent(startPlatform, "Platform", Platform())
ECS.addComponent(startPlatform, "Color", Color(0.5, 0.5, 0.5))
local startTrans = ECS.getComponent(startPlatform, "Transform")
startTrans.sx = 6
startTrans.sy = 1
startTrans.sz = 6

-- ============================================
-- Create Platforms (Path to finish)
-- ============================================

-- Platform 2
local platform2 = ECS.createEntity()
ECS.addComponent(platform2, "Transform", Transform(8, 1, 0))
ECS.addComponent(platform2, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform2, "Collider", Collider("Box", {4, 1, 4}))
ECS.addComponent(platform2, "Platform", Platform())
ECS.addComponent(platform2, "Color", Color(0.4, 0.4, 0.4))
local p2Trans = ECS.getComponent(platform2, "Transform")
p2Trans.sx = 4
p2Trans.sy = 1
p2Trans.sz = 4

-- Platform 3
local platform3 = ECS.createEntity()
ECS.addComponent(platform3, "Transform", Transform(8, 2, 8))
ECS.addComponent(platform3, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform3, "Collider", Collider("Box", {4, 1, 4}))
ECS.addComponent(platform3, "Platform", Platform())
ECS.addComponent(platform3, "Color", Color(0.4, 0.4, 0.4))
local p3Trans = ECS.getComponent(platform3, "Transform")
p3Trans.sx = 4
p3Trans.sy = 1
p3Trans.sz = 4

-- ============================================
-- Checkpoint (Yellow Cube)
-- ============================================
local checkpoint1 = ECS.createEntity()
ECS.addComponent(checkpoint1, "Transform", Transform(8, 4, 8))
ECS.addComponent(checkpoint1, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(checkpoint1, "Collider", Collider("Box", {1, 2, 1}))
ECS.addComponent(checkpoint1, "Checkpoint", Checkpoint(1))
ECS.addComponent(checkpoint1, "Color", Color(1.0, 1.0, 0.0))
local cp1Trans = ECS.getComponent(checkpoint1, "Transform")
cp1Trans.sy = 2

-- Platform 4
local platform4 = ECS.createEntity()
ECS.addComponent(platform4, "Transform", Transform(0, 3, 12))
ECS.addComponent(platform4, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform4, "Collider", Collider("Box", {4, 1, 4}))
ECS.addComponent(platform4, "Platform", Platform())
ECS.addComponent(platform4, "Color", Color(0.4, 0.4, 0.4))
local p4Trans = ECS.getComponent(platform4, "Transform")
p4Trans.sx = 4
p4Trans.sy = 1
p4Trans.sz = 4

-- Platform 5
local platform5 = ECS.createEntity()
ECS.addComponent(platform5, "Transform", Transform(-8, 4, 12))
ECS.addComponent(platform5, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(platform5, "Collider", Collider("Box", {4, 1, 4}))
ECS.addComponent(platform5, "Platform", Platform())
ECS.addComponent(platform5, "Color", Color(0.4, 0.4, 0.4))
local p5Trans = ECS.getComponent(platform5, "Transform")
p5Trans.sx = 4
p5Trans.sy = 1
p5Trans.sz = 4

-- ============================================
-- Finish Platform
-- ============================================
local finishPlatform = ECS.createEntity()
ECS.addComponent(finishPlatform, "Transform", Transform(-8, 5, 20))
ECS.addComponent(finishPlatform, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(finishPlatform, "Collider", Collider("Box", {5, 1, 5}))
ECS.addComponent(finishPlatform, "Platform", Platform())
ECS.addComponent(finishPlatform, "Color", Color(0.3, 0.7, 0.3))
local fpTrans = ECS.getComponent(finishPlatform, "Transform")
fpTrans.sx = 5
fpTrans.sy = 1
fpTrans.sz = 5

-- ============================================
-- Finish Line (Green Cube)
-- ============================================
local finishLine = ECS.createEntity()
ECS.addComponent(finishLine, "Transform", Transform(-8, 7, 20))
ECS.addComponent(finishLine, "Mesh", Mesh("assets/models/cube.obj"))
ECS.addComponent(finishLine, "Collider", Collider("Box", {2, 3, 2}))
ECS.addComponent(finishLine, "FinishLine", FinishLine("assets/scripts/platformer/levels/Level2.lua"))
ECS.addComponent(finishLine, "Color", Color(0.0, 1.0, 0.0))
local flTrans = ECS.getComponent(finishLine, "Transform")
flTrans.sy = 3

-- ============================================
-- Death Floor (Large invisible trigger below the level)
-- ============================================
local deathFloor = ECS.createEntity()
ECS.addComponent(deathFloor, "Transform", Transform(0, -15, 10))
ECS.addComponent(deathFloor, "Collider", Collider("Box", {100, 1, 100}))
ECS.addComponent(deathFloor, "DeathFloor", DeathFloor())

print("[Level1] Level 1 loaded successfully!")
print("[Level1] Use ZQSD to move, LEFT/RIGHT arrows to turn, SPACE to jump")
print("[Level1] Reach the green cube to complete the level!")
