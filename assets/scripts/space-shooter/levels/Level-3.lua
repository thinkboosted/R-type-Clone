
CurrentLevel = 3

local file = io.open("current_level.txt", "w")
if file then
    file:write("3")
    file:close()
end

local isSoloMode = not (ECS.capabilities and ECS.capabilities.hasNetworkSync)
local backgroundTexture

backgroundTexture = "assets/textures/Background/Starfield.png"

local Spawns = require("assets/scripts/space-shooter/spawns")

Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)


print("[Level-3] Level 3 entities loaded!")
