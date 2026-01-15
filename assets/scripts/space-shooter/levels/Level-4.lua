

CurrentLevel = 4

local file = io.open("current_level.txt", "w")
if file then
    file:write("4")
    file:close()
end



local backgroundTexture = "assets/textures/Background/SinglePlay4.png"

local Spawns = require("assets/scripts/space-shooter/spawns")

Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)

print("[Level-1] Level 4 entities loaded!")