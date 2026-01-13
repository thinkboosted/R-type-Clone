


CurrentLevel = 3

local file = io.open("current_level.txt", "w")
if file then
    file:write("3")
    file:close()
end

local Spawns = require("assets/scripts/space-shooter/spawns")

-- Create Backgrounds for this specific level
Spawns.createBackground("assets/textures/Background/SinglePlay3.png")

Spawns.createScore(0)

print("[Level-3] Level 3 entities loaded!")