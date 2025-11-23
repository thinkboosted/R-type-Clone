-- Player Input System
local PlayerInputSystem = {}

function PlayerInputSystem.init()
    print("[PlayerInputSystem] Initialized")
end

function PlayerInputSystem.onKeyPressed(key)
    local players = ECS.getEntitiesWith({"Player", "Physic"})
    for _, id in ipairs(players) do
        local player = ECS.getComponent(id, "Player")
        local physic = ECS.getComponent(id, "Physic")

        local speed = player.speed

        if key == "Z" or key == "W" then
            physic.vz = speed
        elseif key == "S" then
            physic.vz = -speed
        elseif key == "Q" or key == "A" then
            physic.vx = -speed
        elseif key == "D" then
            physic.vx = speed
        elseif key == "SPACE" then
            physic.vy = 5.0
        end
    end
end

function PlayerInputSystem.onMouseMoved(msg)
    -- msg format: "x,y"
end

function PlayerInputSystem.update(dt)
    local players = ECS.getEntitiesWith({"Player", "Physic"})
    for _, id in ipairs(players) do
        local physic = ECS.getComponent(id, "Physic")
        physic.vx = physic.vx * 0.9
        physic.vz = physic.vz * 0.9
    end
end

ECS.registerSystem(PlayerInputSystem)
