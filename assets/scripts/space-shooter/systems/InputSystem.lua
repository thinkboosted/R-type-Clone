local InputSystem = {}

function InputSystem.init()
    print("[InputSystem] Initialized")
    -- AJOUT CRUCIAL : On s'abonne aux événements clavier
    ECS.subscribe("KeyPressed", InputSystem.onKeyPressed)
    ECS.subscribe("KeyReleased", InputSystem.onKeyReleased)
end

function InputSystem.update(dt)
    -- REMOVED GUARD: Update must run on Server (to process network inputs) AND Client (for prediction)
    -- if not ECS.capabilities.hasLocalInput then return end

    local entities = ECS.getEntitiesWith({"InputState", "Physic", "Player", "Transform"})

    for _, id in ipairs(entities) do
        local input = ECS.getComponent(id, "InputState")
        local physic = ECS.getComponent(id, "Physic")
        local player = ECS.getComponent(id, "Player")
        local transform = ECS.getComponent(id, "Transform")
        local speed = player.speed or 10.0

        -- Reset Vitesse
        physic.vx = 0
        physic.vy = 0

        -- Application des inputs stockés dans le composant InputState
        local moveX = 0
        local moveY = 0

        if input.left then moveX = moveX - 1 end
        if input.right then moveX = moveX + 1 end
        if input.up then moveY = moveY + 1 end
        if input.down then moveY = moveY - 1 end

        physic.vx = moveX * speed
        physic.vy = moveY * speed

        -- CLIENT PREDICTION: If we don't have authority (Client), apply movement immediately
        if not ECS.capabilities.hasAuthority then
             transform.x = transform.x + physic.vx * dt
             transform.y = transform.y + physic.vy * dt
        end
    end
end

function InputSystem.onKeyPressed(key)
    if not ECS.capabilities.hasLocalInput then return end

    -- Network Sync: Send Input to Server
    if ECS.capabilities.hasNetworkSync and not ECS.capabilities.hasAuthority then
        ECS.sendNetworkMessage("INPUT", key .. " 1")
    end

    local entities = ECS.getEntitiesWith({"InputState"})
    for _, id in ipairs(entities) do
        local input = ECS.getComponent(id, "InputState")
        if key == "UP" or key == "Z" or key == "W" then input.up = true end
        if key == "DOWN" or key == "S" then input.down = true end
        if key == "LEFT" or key == "Q" or key == "A" then input.left = true end
        if key == "RIGHT" or key == "D" then input.right = true end
        if key == "SPACE" then input.shoot = true end
    end
end

function InputSystem.onKeyReleased(key)
    if not ECS.capabilities.hasLocalInput then return end
    

    -- Network Sync: Send Input to Server
    if ECS.capabilities.hasNetworkSync and not ECS.capabilities.hasAuthority then
        ECS.sendNetworkMessage("INPUT", key .. " 0")
    end

    local entities = ECS.getEntitiesWith({"InputState"})
    for _, id in ipairs(entities) do
        local input = ECS.getComponent(id, "InputState")
        if key == "UP" or key == "Z" or key == "W" then input.up = false end
        if key == "DOWN" or key == "S" then input.down = false end
        if key == "LEFT" or key == "Q" or key == "A" then input.left = false end
        if key == "RIGHT" or key == "D" then input.right = false end
        if key == "SPACE" then input.shoot = false end
    end
    
    if key == "ESCAPE" then
        ECS.sendMessage("ExitApplication", "")
    end
end

ECS.registerSystem(InputSystem)
return InputSystem