-- Player Input System for 3D Platformer
PlayerSystem = {}

PlayerSystem.keys = {
    forward = false,
    backward = false,
    left = false,
    right = false,
    rotateLeft = false,
    rotateRight = false,
    jump = false
}

PlayerSystem.jumpRequested = false

local function getHostControlledPlayer()
    local players = ECS.getEntitiesWith({"Player", "Physic", "Transform"})
    if #players == 0 then return nil end

    if _G.PlatformerNetwork and _G.PlatformerNetwork.hostPlayerId then
        return _G.PlatformerNetwork.hostPlayerId
    end

    for _, id in ipairs(players) do
        local remote = ECS.getComponent(id, "RemoteControl")
        if not remote then
            return id
        end
    end

    return players[1]
end

local function getClientControlledPlayer()
    local net = _G.PlatformerNetwork
    if net and net.assignedServerPlayerId then
        local mapped = net.serverToLocal[net.assignedServerPlayerId]
        if mapped then
            return mapped
        end
    end

    local players = ECS.getEntitiesWith({"Player", "Physic", "Transform"})
    if #players > 0 then
        return players[1]
    end
    return nil
end

local function sendClientInput(key, pressed)
    if not ECS.capabilities.hasNetworkSync or ECS.capabilities.hasAuthority then
        return
    end
    ECS.sendNetworkMessage("PINPUT", key .. " " .. (pressed and "1" or "0"))
end

function PlayerSystem.init()
    print("[PlayerSystem] Initialized")
    -- Subscribe to keyboard events
    ECS.subscribe("KeyPressed", PlayerSystem.onKeyPressed)
    ECS.subscribe("KeyReleased", PlayerSystem.onKeyReleased)
end

function PlayerSystem.update(dt)
    local id = nil
    if ECS.capabilities.hasAuthority then
        id = getHostControlledPlayer()
    else
        id = getClientControlledPlayer()
    end
    if not id then return end

    local physic = ECS.getComponent(id, "Physic")
    local transform = ECS.getComponent(id, "Transform")
    local playerComp = ECS.getComponent(id, "Player")
    if not physic or not transform or not playerComp then return end

    -- Handle rotation with left/right arrows
    local rotationSpeed = 3.0
    local vaz = 0
    if PlayerSystem.keys.rotateLeft then
        vaz = vaz + rotationSpeed
    end
    if PlayerSystem.keys.rotateRight then
        vaz = vaz - rotationSpeed
    end
    if ECS.capabilities.hasAuthority then
        physic.vaz = vaz
    end

    -- Calculate movement direction based on player's rotation
    local speed = playerComp.speed
    local ry = transform.ry
    local forwardX = math.sin(ry)
    local forwardZ = math.cos(ry)
    local rightX = math.cos(ry)
    local rightZ = -math.sin(ry)

    local vx = 0
    local vz = 0

    -- ZQSD movement
    if PlayerSystem.keys.forward then
        vx = vx + forwardX
        vz = vz + forwardZ
    end
    if PlayerSystem.keys.backward then
        vx = vx - forwardX
        vz = vz - forwardZ
    end
    if PlayerSystem.keys.left then
        vx = vx + rightX
        vz = vz + rightZ
    end
    if PlayerSystem.keys.right then
        vx = vx - rightX
        vz = vz - rightZ
    end

    -- Normalize diagonal movement
    local length = math.sqrt(vx * vx + vz * vz)
    if length > 0 then
        vx = (vx / length) * speed
        vz = (vz / length) * speed
    end

    if ECS.capabilities.hasAuthority then
        physic.vx = vx
        physic.vz = vz
    end

    -- Update jump cooldown
    if ECS.capabilities.hasAuthority and playerComp.jumpCooldown > 0 then
        playerComp.jumpCooldown = playerComp.jumpCooldown - dt
    end

    -- Handle jump (only if grounded, cooldown expired, and jump requested)
    if ECS.capabilities.hasAuthority and PlayerSystem.jumpRequested and playerComp.isGrounded and playerComp.jumpCooldown <= 0 then
        ECS.sendMessage("PhysicCommand", "ApplyImpulse:" .. id .. ":0," .. playerComp.jumpForce .. ",0;")
        playerComp.isGrounded = false
        playerComp.jumpCooldown = playerComp.jumpCooldownTime
        PlayerSystem.jumpRequested = false
        
        -- Play jump sound
        ECS.sendMessage("SoundPlay", "jump:effects/hit.wav:70")
    end
end

function PlayerSystem.onKeyPressed(key)
    if key == "Z" or key == "W" then
        PlayerSystem.keys.forward = true
    elseif key == "S" then
        PlayerSystem.keys.backward = true
    elseif key == "Q" or key == "A" then
        PlayerSystem.keys.left = true
    elseif key == "D" then
        PlayerSystem.keys.right = true
    elseif key == "LEFT" then
        PlayerSystem.keys.rotateLeft = true
    elseif key == "RIGHT" then
        PlayerSystem.keys.rotateRight = true
    elseif key == "SPACE" then
        PlayerSystem.jumpRequested = true
    end

    sendClientInput(key, true)
end

function PlayerSystem.onKeyReleased(key)
    if key == "Z" or key == "W" then
        PlayerSystem.keys.forward = false
    elseif key == "S" then
        PlayerSystem.keys.backward = false
    elseif key == "Q" or key == "A" then
        PlayerSystem.keys.left = false
    elseif key == "D" then
        PlayerSystem.keys.right = false
    elseif key == "LEFT" then
        PlayerSystem.keys.rotateLeft = false
    elseif key == "RIGHT" then
        PlayerSystem.keys.rotateRight = false
    elseif key == "SPACE" then
        PlayerSystem.jumpRequested = false
    end

    sendClientInput(key, false)
end

ECS.registerSystem(PlayerSystem)
