-- Player Input System
local PlayerInputSystem = {}

PlayerInputSystem.keys = {
    forward = false,
    backward = false,
    left = false,
    right = false,
    rotateLeft = false,
    rotateRight = false
}

function PlayerInputSystem.init()
    print("[PlayerInputSystem] Initialized")
end

function PlayerInputSystem.update(dt)
    local players = ECS.getEntitiesWith({"Player", "Physic", "Transform"})
    if #players == 0 then return end

    local id = players[1]
    local physic = ECS.getComponent(id, "Physic")
    local transform = ECS.getComponent(id, "Transform")
    local playerComp = ECS.getComponent(id, "Player")

    local rotationSpeed = playerComp.speed * 0.2
    local vaz = 0
    if PlayerInputSystem.keys.rotateLeft then
        vaz = vaz + rotationSpeed
    end
    if PlayerInputSystem.keys.rotateRight then
        vaz = vaz - rotationSpeed
    end
    physic.vaz = vaz

    local speed = playerComp.speed
    local ry = transform.ry
    local forwardX = math.sin(ry)
    local forwardZ = math.cos(ry)
    local rightX = math.cos(ry)
    local rightZ = -math.sin(ry)

    local vx = 0
    local vz = 0

    if PlayerInputSystem.keys.forward then
        vx = vx + forwardX
        vz = vz + forwardZ
    end
    if PlayerInputSystem.keys.backward then
        vx = vx - forwardX
        vz = vz - forwardZ
    end
    if PlayerInputSystem.keys.left then
        vx = vx + rightX
        vz = vz + rightZ
    end
    if PlayerInputSystem.keys.right then
        vx = vx - rightX
        vz = vz - rightZ
    end

    physic.vx = vx * speed
    physic.vz = vz * speed
end

function PlayerInputSystem.onKeyPressed(key)
    if key == "Z" or key == "W" then
        PlayerInputSystem.keys.forward = true
    elseif key == "S" then
        PlayerInputSystem.keys.backward = true
    elseif key == "Q" or key == "A" then
        PlayerInputSystem.keys.left = true
    elseif key == "D" then
        PlayerInputSystem.keys.right = true
    elseif key == "LEFT" then
        PlayerInputSystem.keys.rotateLeft = true
    elseif key == "RIGHT" then
        PlayerInputSystem.keys.rotateRight = true
    end
end

function PlayerInputSystem.onKeyReleased(key)
    local players = ECS.getEntitiesWith({"Player", "Physic", "Transform"})
    if #players == 0 then return end
    local id = players[1]
    local player = ECS.getComponent(id, "Player")

    if key == "Z" or key == "W" then
        PlayerInputSystem.keys.forward = false
    elseif key == "S" then
        PlayerInputSystem.keys.backward = false
    elseif key == "Q" or key == "A" then
        PlayerInputSystem.keys.left = false
    elseif key == "D" then
        PlayerInputSystem.keys.right = false
    elseif key == "LEFT" then
        PlayerInputSystem.keys.rotateLeft = false
    elseif key == "RIGHT" then
        PlayerInputSystem.keys.rotateRight = false
    elseif key == "SPACE" then
        local jumpImpulse = 5 * player.speed;
        local msg = "ApplyImpulse:" .. id .. ":0," .. jumpImpulse .. ",0;"
        ECS.sendMessage("PhysicCommand", msg)
    elseif key == "ESCAPE" then
        ECS.sendMessage("ExitApplication", "")
    end
end

function PlayerInputSystem.onMouseMoved(msg)
    -- msg format: "x,y"
end

ECS.registerSystem(PlayerInputSystem)
