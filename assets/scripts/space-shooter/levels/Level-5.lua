

CurrentLevel = 5

local file = io.open("current_level.txt", "w")
if file then
    file:write("5")
    file:close()
end

-- Conditional rendering: Check if solo or multiplayer
local isSoloMode = not (ECS.capabilities and ECS.capabilities.hasNetworkSync)
local backgroundTexture

if isSoloMode then
    backgroundTexture = "assets/textures/Background/StartSky.jpg"
else
    -- Level 5 doesn't have a specific multiplayer background, using the same
    backgroundTexture = "assets/textures/Background/StartSky.jpg"
end

local Spawns = require("assets/scripts/space-shooter/spawns")


Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)


print("[Level-5] Level 5 entities loaded!")