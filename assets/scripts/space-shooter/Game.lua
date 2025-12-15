-- Game Definition

dofile("assets/scripts/space-shooter/components/Components.lua")

-- Global Flags
ECS.isGameRunning = false
ECS.isLocal = false

-- Always load NetworkSystem (it handles its own logic based on isServer)
dofile("assets/scripts/space-shooter/systems/NetworkSystem.lua")

if ECS.isServer() then
    dofile("assets/scripts/space-shooter/GameServer.lua")
else
    dofile("assets/scripts/space-shooter/GameClient.lua")
end

print("Space Shooter Game Loaded")
