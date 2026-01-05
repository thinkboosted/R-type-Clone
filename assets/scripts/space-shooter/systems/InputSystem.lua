local config = dofile("assets/scripts/space-shooter/config.lua")
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
        local bounds = config.player.boundaries

        if input.left and transform.x > bounds.minX then moveX = moveX - 1 end
        if input.right and transform.x < bounds.maxX then moveX = moveX + 1 end
        if input.up and transform.y < bounds.maxY then moveY = moveY + 1 end
        if input.down and transform.y > bounds.minY then moveY = moveY - 1 end

        physic.vx = moveX * speed
        physic.vy = moveY * speed

        -- CLIENT PREDICTION: If we don't have authority (Client), apply movement immediately
        if not ECS.capabilities.hasAuthority then
             transform.x = transform.x + physic.vx * dt
             transform.y = transform.y + physic.vy * dt

             -- Hard Clamp for visual smoothness
             if transform.x < bounds.minX then transform.x = bounds.minX end
             if transform.x > bounds.maxX then transform.x = bounds.maxX end
             if transform.y < bounds.minY then transform.y = bounds.minY end
             if transform.y > bounds.maxY then transform.y = bounds.maxY end
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
        if key == "UP" then input.up = true end
        if key == "DOWN" then input.down = true end
        if key == "LEFT" then input.left = true end
        if key == "RIGHT" then input.right = true end
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
        if key == "UP" then input.up = false end
        if key == "DOWN" then input.down = false end
        if key == "LEFT" then input.left = false end
        if key == "RIGHT" then input.right = false end
        if key == "SPACE" then input.shoot = false end
    end

    if key == "ESCAPE" then
        ECS.sendMessage("ExitApplication", "")
    end
end

ECS.registerSystem(InputSystem)
return InputSystem