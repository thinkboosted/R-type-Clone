

CurrentLevel = 5

local file = io.open("current_level.txt", "w")
if file then
    file:write("5")
    file:close()
end

local Spawns = require("assets/scripts/space-shooter/spawns")

-- Create Backgrounds for this specific level
Spawns.createBackground("assets/textures/Background/StartSky.jpg")

Spawns.createScore(0)

print("[Level-5] Level 5 entities loaded!")