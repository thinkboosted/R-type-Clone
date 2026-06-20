-- Game Definition Level 2

CurrentLevel = 2

local file = io.open("current_level.txt", "w")
if file then
    file:write("2")
    file:close()
end

local isSoloMode = not (ECS.capabilities and ECS.capabilities.hasNetworkSync)
local backgroundTexture

backgroundTexture = "assets/textures/Background/Starfield.png"

local Spawns = require("assets/scripts/space-shooter/spawns")

Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)
ECS.sendMessage("ShowLevelIntro", "2")

print("Level 2 loaded!")
