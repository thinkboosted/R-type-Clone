
CurrentLevel = 3

local file = io.open("current_level.txt", "w")
if file then
    file:write("3")
    file:close()
end

-- Conditional rendering: Check if solo or multiplayer
local isSoloMode = not (ECS.capabilities and ECS.capabilities.hasNetworkSync)
local backgroundTexture

if isSoloMode then
    backgroundTexture = "assets/textures/Background/SinglePlay3.png"
else
    backgroundTexture = "assets/textures/Background/Multiplayer_3.png"
end

local Spawns = require("assets/scripts/space-shooter/spawns")

Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)


print("[Level-3] Level 3 entities loaded!")