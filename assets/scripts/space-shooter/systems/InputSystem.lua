local InputSystem = {}

InputSystem.keys = {
    up = false,
    down = false,
    left = false,
    right = false,
    shoot = false
}

function InputSystem.init()
    print("[InputSystem] Initialized")
end

function InputSystem.update(dt)
    -- Only the server (authoritative) or local solo mode should step movement/physics.
    if not ECS.isServer() and not ECS.isLocal then
        return
    end

    local entities = ECS.getEntitiesWith({"Player", "Physic", "InputState", "Transform"})
    for _, id in ipairs(entities) do
        local physic = ECS.getComponent(id, "Physic")
        local transform = ECS.getComponent(id, "Transform")
        local playerComp = ECS.getComponent(id, "Player")
        local input = ECS.getComponent(id, "InputState")
        local weapon = ECS.getComponent(id, "Weapon")

        if weapon then
            weapon.timeSinceLastShot = weapon.timeSinceLastShot + dt
        end

        -- Movement
        local speed = playerComp.speed
        local vx = 0
        local vy = 0

        if input.up then vy = vy + 1 end
        if input.down then vy = vy - 1 end
        if input.left then vx = vx - 1 end
        if input.right then vx = vx + 1 end

        physic.vx = vx * speed
        physic.vy = vy * speed
        physic.vz = 0
        
        -- Apply velocity to Transform immediately for solo mode.
        if ECS.isLocal then
            transform.x = transform.x + physic.vx * dt
            transform.y = transform.y + physic.vy * dt
        end

        -- Shooting (server/local only)
        if input.shoot and (ECS.isServer() or ECS.isLocal) then
            local canShoot = true
            if weapon then
                if weapon.timeSinceLastShot < weapon.cooldown then
                    canShoot = false
                else
                    weapon.timeSinceLastShot = 0
                end
            end

            if canShoot then
                 InputSystem.spawnBullet(transform.x + 1.5, transform.y, transform.z)
            end
        end
    end
end

function InputSystem.spawnBullet(x, y, z)
    -- Bullet Creation
    local bullet = ECS.createEntity()
    ECS.addComponent(bullet, "Transform", Transform(x, y, z))
    ECS.addComponent(bullet, "Collider", Collider("Sphere", {0.2}))
    ECS.addComponent(bullet, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(bullet, "Bullet", Bullet(10))
    ECS.addComponent(bullet, "Life", Life(1))
    
    if ECS.isLocal then
        -- Add visuals for local play
        ECS.addComponent(bullet, "Mesh", Mesh("assets/models/cube.obj"))
        ECS.addComponent(bullet, "Color", Color(1.0, 1.0, 0.0))
        local t = ECS.getComponent(bullet, "Transform")
        t.sx = 0.2; t.sy = 0.2; t.sz = 0.2
    end

    local p = ECS.getComponent(bullet, "Physic")
    p.vx = 20.0
end

function InputSystem.onKeyPressed(key)
    if not ECS.isServer() and not ECS.isLocal then
        -- Pure client: only send to server, do not mutate local input state.
        ECS.sendNetworkMessage("INPUT", key .. " 1")
    elseif ECS.isLocal then
        -- Local Input Handling
        local players = ECS.getEntitiesWith({"InputState"})
        for _, id in ipairs(players) do
            local input = ECS.getComponent(id, "InputState")
            if key == "UP" or key == "Z" or key == "W" then input.up = true end
            if key == "DOWN" or key == "S" then input.down = true end
            if key == "LEFT" or key == "Q" or key == "A" then input.left = true end
            if key == "RIGHT" or key == "D" then input.right = true end
            if key == "SPACE" then input.shoot = true end
        end
    end
end

function InputSystem.onKeyReleased(key)
    if not ECS.isServer() and not ECS.isLocal then
        ECS.sendNetworkMessage("INPUT", key .. " 0")
    elseif ECS.isLocal then
        -- Local Input Handling
        local players = ECS.getEntitiesWith({"InputState"})
        for _, id in ipairs(players) do
            local input = ECS.getComponent(id, "InputState")
            if key == "UP" or key == "Z" or key == "W" then input.up = false end
            if key == "DOWN" or key == "S" then input.down = false end
            if key == "LEFT" or key == "Q" or key == "A" then input.left = false end
            if key == "RIGHT" or key == "D" then input.right = false end
            if key == "SPACE" then input.shoot = false end
        end
    end
    
    if key == "ESCAPE" then
        ECS.sendMessage("ExitApplication", "")
    end
end

function InputSystem.onMouseMoved(x, y)
end

function InputSystem.onMousePressed(button)
end

function InputSystem.onMouseReleased(button)
end

ECS.registerSystem(InputSystem)