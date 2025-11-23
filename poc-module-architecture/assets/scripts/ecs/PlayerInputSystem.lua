-- Player Input System
local PlayerInputSystem = {}

function PlayerInputSystem.init()
    print("[PlayerInputSystem] Initialized")
end

function PlayerInputSystem.onKeyPressed(key)
    local players = ECS.getEntitiesWith({"Player", "Physic", "Transform"})
    for _, id in ipairs(players) do
        local player = ECS.getComponent(id, "Player")
        local physic = ECS.getComponent(id, "Physic")
        local transform = ECS.getComponent(id, "Transform")

        local speed = player.speed

        -- Calculate forward/right vectors based on rotation (Y-axis rotation)
        local ry = transform.ry
        local forwardX = math.sin(ry)
        local forwardZ = math.cos(ry)
        local rightX = math.cos(ry)
        local rightZ = -math.sin(ry)

        if key == "Z" or key == "W" then
            physic.vx = forwardX * speed
            physic.vz = forwardZ * speed
        elseif key == "S" then
            physic.vx = -forwardX * speed
            physic.vz = -forwardZ * speed
        elseif key == "Q" or key == "A" then
            physic.vx = -rightX * speed
            physic.vz = -rightZ * speed
        elseif key == "D" then
            physic.vx = rightX * speed
            physic.vz = rightZ * speed
        elseif key == "SPACE" then
            physic.vy = 10.0
        elseif key == "LEFT" then
            player.inputX = 1
        elseif key == "RIGHT" then
            player.inputX = -1
        elseif key == "UP" then
            player.inputY = 1
        elseif key == "DOWN" then
            player.inputY = -1
        end
    end
end

function PlayerInputSystem.onKeyReleased(key)
    local players = ECS.getEntitiesWith({"Player", "Physic"})
    for _, id in ipairs(players) do
        local player = ECS.getComponent(id, "Player")
        local physic = ECS.getComponent(id, "Physic")

        if key == "UP" or key == "DOWN" then
            player.inputY = 0
        elseif key == "LEFT" or key == "RIGHT" then
            player.inputX = 0
            physic.vay = 0
        end
    end
end

function PlayerInputSystem.onMouseMoved(msg)
    -- msg format: "x,y"
end

function PlayerInputSystem.update(dt)
    local players = ECS.getEntitiesWith({"Player", "Physic"})
    for _, id in ipairs(players) do
        local player = ECS.getComponent(id, "Player")
        local physic = ECS.getComponent(id, "Physic")

        -- Apply rotation based on inputX
        if player.inputX ~= 0 then
            physic.vay = player.inputX * 2.0
        end

        physic.vx = physic.vx * 0.9
        physic.vz = physic.vz * 0.9
    end
end

ECS.registerSystem(PlayerInputSystem)
