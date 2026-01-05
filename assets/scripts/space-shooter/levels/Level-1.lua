-- Game Definition
-- Level-1.lua: Defines the entities for Level 1
-- This file is loaded by MenuSystem.lua when starting a SOLO game

CurrentLevel = 1

local file = io.open("current_level.txt", "w")
if file then
    file:write("1")
    file:close()
end

local Spawns = require("assets/scripts/space-shooter/spawns")

-- Create Backgrounds (Parallax)
Spawns.createBackground("assets/textures/PlutoImage_TutorialLevel.png")

-- Create Score Entity
local scoreEntity = ECS.createEntity()
ECS.addComponent(scoreEntity, "Score", Score(0))
ECS.addComponent(scoreEntity, "Transform", Transform(650, 550, 0))
ECS.addComponent(scoreEntity, "Text", Text("Score: 0", "assets/fonts/arial.ttf", 24, true))
ECS.addComponent(scoreEntity, "Color", Color(1.0, 1.0, 1.0))

print("[Level-1] Level 1 entities loaded!")
