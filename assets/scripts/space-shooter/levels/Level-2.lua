-- Game Definition Level 2

CurrentLevel = 2

local file = io.open("current_level.txt", "w")
if file then
    file:write("2")
    file:close()
end

local isSoloMode = not (ECS.capabilities and ECS.capabilities.hasNetworkSync)
local backgroundTexture

if isSoloMode then
    backgroundTexture = "assets/textures/Background/SinglePlay2.png"
else
    backgroundTexture = "assets/textures/Background/Multiplayer_2.png"
end

local Spawns = require("assets/scripts/space-shooter/spawns")

Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)

print("Level 2 loaded!")
