-- Game Definition
-- Level-1.lua: Defines the entities for Level 1
-- This file is loaded by MenuSystem.lua when starting a SOLO game

CurrentLevel = 1

local file = io.open("current_level.txt", "w")
if file then
    file:write("1")
    file:close()
end


local backgroundTexture = "assets/textures/Background/SinglePlay1.png"

local Spawns = require("assets/scripts/space-shooter/spawns")


Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)

print("[Level-1] Level 1 entities loaded!")
