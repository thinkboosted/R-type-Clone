
-- entry point of the game

local file = io.open("current_level.txt", "r")
local level = 1
if file then
    local content = file:read("*all")
    level = tonumber(content) or 1
    file:close()
end

print("Loading Level " .. level)

if level == 1  then
    dofile("assets/scripts/space-shooter/levels/Level-1.lua")
else
    dofile("assets/scripts/space-shooter/levels/Level-" .. level ..".lua")
end

