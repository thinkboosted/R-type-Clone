

CurrentLevel = 4

local file = io.open("current_level.txt", "w")
if file then
    file:write("4")
    file:close()
end

-- Conditional rendering: Check if solo or multiplayer
local isSoloMode = not (ECS.capabilities and ECS.capabilities.hasNetworkSync)
local backgroundTexture

backgroundTexture = "assets/textures/Background/Starfield.png"

local Spawns = require("assets/scripts/space-shooter/spawns")

Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)

print("[Level-1] Level 4 entities loaded!")
