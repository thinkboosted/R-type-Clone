-- Game Definition Level 2

CurrentLevel = 2

local file = io.open("current_level.txt", "w")
if file then
    file:write("2")
    file:close()
end

local backgroundTexture = "assets/textures/Background/SinglePlay2.png"

local Spawns = require("assets/scripts/space-shooter/spawns")


Spawns.createCoreEntities(CurrentLevel, backgroundTexture, CurrentScore)


-- (For Multi)
-- -- Create Player Tag
-- local playerTag = ECS.createEntity()
-- ECS.addComponent(playerTag, "Transform", Transform(0, 0, 0))
-- ECS.addComponent(playerTag, "Text", Text("Player1", "assets/fonts/arial.ttf", 24, false))
-- ECS.addComponent(playerTag, "Color", Color(1.0, 1.0, 0.0))
-- ECS.addComponent(playerTag, "Follow", Follow(player, 0, 2.5, 0))

print("Level 2 loaded!")
