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

backgroundTexture = "assets/textures/Background/Starfield.png"

local Spawns = require("assets/scripts/space-shooter/spawns")


Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)
ECS.sendMessage("ShowLevelIntro", "1")

print("[Level-1] Level 1 entities loaded!")
