-- Main.lua - 3D Platformer Entry Point
-- This script initializes the game systems and loads the first level

print("===========================================")
print("    3D PLATFORMER - Starting Game")
print("===========================================")
print("")

-- Ensure solo capability defaults when no network handshake happened.
if not ECS.capabilities.hasNetworkSync and not ECS.capabilities.hasAuthority then
	ECS.setGameMode("SOLO")
end

print("[Main] Capabilities: authority=" .. tostring(ECS.capabilities.hasAuthority)
	.. " network=" .. tostring(ECS.capabilities.hasNetworkSync)
	.. " render=" .. tostring(ECS.capabilities.hasRendering))

-- Load component definitions
dofile("assets/scripts/platformer/Components.lua")
print("[Main] Components loaded")

-- Load all game systems
dofile("assets/scripts/platformer/systems/CollisionSystem.lua")
print("[Main] CollisionSystem loaded")

dofile("assets/scripts/platformer/systems/PhysicSystem.lua")
print("[Main] PhysicSystem loaded")

dofile("assets/scripts/platformer/systems/PlayerSystem.lua")
print("[Main] PlayerSystem loaded")

dofile("assets/scripts/platformer/systems/CameraSystem.lua")
print("[Main] CameraSystem loaded")

dofile("assets/scripts/platformer/systems/RenderSystem.lua")
print("[Main] RenderSystem loaded")

dofile("assets/scripts/platformer/systems/GameLogicSystem.lua")
print("[Main] GameLogicSystem loaded")

if ECS.capabilities.hasNetworkSync then
	dofile("assets/scripts/platformer/systems/NetworkSystem.lua")
	print("[Main] NetworkSystem loaded")
end

print("")
print("[Main] All systems loaded successfully!")
print("")

-- Load the first level
print("[Main] Loading Level 1...")
dofile("assets/scripts/platformer/levels/Level1.lua")

print("")
print("===========================================")
print("    Controls:")
print("    Z/W - Move forward")
print("    S   - Move backward")
print("    Q/A - Strafe left")
print("    D   - Strafe right")
print("    LEFT/RIGHT arrows - Turn")
print("    SPACE - Jump")
print("===========================================")
print("")
