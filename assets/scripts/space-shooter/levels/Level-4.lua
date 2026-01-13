

CurrentLevel = 4

local file = io.open("current_level.txt", "w")
if file then
    file:write("4")
    file:close()
end

local Spawns = require("assets/scripts/space-shooter/spawns")

-- Create Backgrounds for this specific level
Spawns.createBackground("assets/textures/Background/SinglePlay4.png")

Spawns.createScore(0)

print("[Level-1] Level 4 entities loaded!")