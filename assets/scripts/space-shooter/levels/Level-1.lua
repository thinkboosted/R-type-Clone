-- Game Definition
-- Level-1.lua: Defines the entities for Level 1
-- This file is loaded by MenuSystem.lua when starting a SOLO game

CurrentLevel = 1

local file = io.open("current_level.txt", "w")
if file then
    file:write("1")
    file:close()
end

-- Conditional rendering: Check if solo or multiplayer
local isSoloMode = not (ECS.capabilities and ECS.capabilities.hasNetworkSync)
local backgroundTexture

if isSoloMode then
    backgroundTexture = "assets/textures/Background/SinglePlay1.png"
else
    backgroundTexture = "assets/textures/Background/Multiplayer_1.png"
end

local Spawns = require("assets/scripts/space-shooter/spawns")


Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)

print("[Level-1] Level 1 entities loaded!")
