
local InputSystem = {}

InputSystem.keys = {
    up = false,
    down = false,
    left = false,
    right = false,
    shoot = false
}

InputSystem.timeSinceLastShoot = 0
InputSystem.shootCooldown = 0.2
InputSystem.blinkTimer = 0

function InputSystem.init()
    print("[InputSystem] Initialized")
end

function InputSystem.update(dt)
    InputSystem.timeSinceLastShoot = InputSystem.timeSinceLastShoot + dt
    InputSystem.blinkTimer = InputSystem.blinkTimer + dt

    local players = ECS.getEntitiesWith({"Player", "Physic", "Transform"})
    if #players == 0 then return end

    local id = players[1]
    local physic = ECS.getComponent(id, "Physic")
    local transform = ECS.getComponent(id, "Transform")
    local playerComp = ECS.getComponent(id, "Player")
    local powerUp = ECS.getComponent(id, "PowerUp")
    local color = ECS.getComponent(id, "Color")


    local activeCooldown = InputSystem.shootCooldown
    if powerUp then
        powerUp.timeRemaining = powerUp.timeRemaining - dt
        if powerUp.timeRemaining <= 0 then

            ECS.removeComponent(id, "PowerUp")
            if color then
                color.r = 0.0
                color.g = 1.0
                color.b = 0.0
            end
        else

            activeCooldown = 0.05

            if color then
                local blinkSpeed = 15.0
                local t = (math.sin(InputSystem.blinkTimer * blinkSpeed) + 1.0) / 2.0
                color.r = 0.0
                color.g = 0.5 + t
                color.b = 0.0 + t * 0.5
            end
        end
    end

    local speed = playerComp.speed
    local vx = 0
    local vy = 0

    if InputSystem.keys.up then
        vy = vy + 1
    end
    if InputSystem.keys.down then
        vy = vy - 1
    end
    if InputSystem.keys.left then
        vx = vx - 1
    end
    if InputSystem.keys.right then
        vx = vx + 1
    end

    physic.vx = vx * speed
    physic.vy = vy * speed
    physic.vz = 0

    if InputSystem.keys.shoot then
        if InputSystem.timeSinceLastShoot > activeCooldown then
            InputSystem.spawnBullet(transform.x + 1.0, transform.y, transform.z)
            InputSystem.timeSinceLastShoot = 0
        end
    end
end

function InputSystem.spawnBullet(x, y, z)
    local bullet = ECS.createEntity()
    ECS.addComponent(bullet, "Transform", Transform(x, y, z))
    ECS.addComponent(bullet, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(bullet, "Collider", Collider("Sphere", {0.2}))
    ECS.addComponent(bullet, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(bullet, "Bullet", Bullet(10))
    ECS.addComponent(bullet, "Life", Life(1))
    ECS.addComponent(bullet, "Color", Color(1.0, 1.0, 0.0))
    ECS.addComponent(bullet, "ParticleGenerator", ParticleGenerator(
        0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        0.3,
        5.0,
        0.5,
        50.0,
        0.1,
        1.0, 0.6, 0.1
    ))
    local t = ECS.getComponent(bullet, "Transform")
    t.sx = 0.2
    t.sy = 0.2
    t.sz = 0.2

    local p = ECS.getComponent(bullet, "Physic")
    p.vx = 20.0
end

function InputSystem.onKeyPressed(key)
    if key == "Z" or key == "W" then
        InputSystem.keys.up = true
    elseif key == "S" then
        InputSystem.keys.down = true
    elseif key == "Q" or key == "A" then
        InputSystem.keys.left = true
    elseif key == "D" then
        InputSystem.keys.right = true
    elseif key == "SPACE" then
        InputSystem.keys.shoot = true
    end
end

function InputSystem.onKeyReleased(key)
    if key == "Z" or key == "W" then
        InputSystem.keys.up = false
    elseif key == "S" then
        InputSystem.keys.down = false
    elseif key == "Q" or key == "A" then
        InputSystem.keys.left = false
    elseif key == "D" then
        InputSystem.keys.right = false
    elseif key == "SPACE" then
        InputSystem.keys.shoot = false
    elseif key == "ESCAPE" then
        ECS.saveState("space-shooter-save")
        ECS.sendMessage("ExitApplication", "")
    end
end

function InputSystem.onMouseMoved(x, y)
end

ECS.registerSystem(InputSystem)
