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
    if ECS.isServer() then
        -- SERVER: Apply Physics based on InputState
        local entities = ECS.getEntitiesWith({"Player", "Physic", "InputState", "Transform"})
        for _, id in ipairs(entities) do
            local physic = ECS.getComponent(id, "Physic")
            local transform = ECS.getComponent(id, "Transform")
            local playerComp = ECS.getComponent(id, "Player")
            local input = ECS.getComponent(id, "InputState")
            local weapon = ECS.getComponent(id, "Weapon")

            -- Cooldown Management
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

            -- Shooting
            if input.shoot then
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
end

function InputSystem.spawnBullet(x, y, z)
    -- Server Side Bullet Creation
    local bullet = ECS.createEntity()
    ECS.addComponent(bullet, "Transform", Transform(x, y, z))
    -- No Mesh on server, just logic
    ECS.addComponent(bullet, "Collider", Collider("Sphere", {0.2}))
    ECS.addComponent(bullet, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(bullet, "Bullet", Bullet(10))
    ECS.addComponent(bullet, "Life", Life(1))

    local p = ECS.getComponent(bullet, "Physic")
    p.vx = 20.0
end

function InputSystem.onKeyPressed(key)
    if not ECS.isServer() then
        ECS.sendNetworkMessage("INPUT", key .. " 1")
    end
end

function InputSystem.onKeyReleased(key)
    if not ECS.isServer() then
        ECS.sendNetworkMessage("INPUT", key .. " 0")

        if key == "ESCAPE" then
            ECS.sendMessage("ExitApplication", "")
        end
    end
end

function InputSystem.onMouseMoved(x, y)
end

function InputSystem.onMousePressed(button)
end

function InputSystem.onMouseReleased(button)
end

ECS.registerSystem(InputSystem)
