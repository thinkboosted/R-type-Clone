-- Component Definitions

function Transform(x, y, z)
    return {
        x = x or 0,
        y = y or 0,
        z = z or 0,
        rx = 0, ry = 0, rz = 0,
        sx = 1, sy = 1, sz = 1
    }
end

function Mesh(modelPath)
    return {
        modelPath = modelPath
    }
end

function Sprite(texturePath, isScreenSpace)
    return {
        texturePath = texturePath,
        isScreenSpace = isScreenSpace or false
    }
end

function Collider(type, size)
    return {
        type = type or "Box",
        size = size or {1, 1, 1}
    }
end

function Physic(mass, friction, fixedRotation, useGravity)
    return {
        mass = mass or 1.0,
        friction = friction or 0.5,
        fixedRotation = fixedRotation or false,
        useGravity = useGravity ~= false, -- Default true
        vx = 0, vy = 0, vz = 0,
        vax = 0, vay = 0, vaz = 0,
        ax = 0, ay = 0, az = 0
    }
end

function Camera(fov)
    return {
        fov = fov or 90.0,
        isActive = true
    }
end

function Color(r, g, b)
    return {
        r = r or 1.0,
        g = g or 1.0,
        b = b or 1.0
    }
end

function Player(speed)
    return {
        speed = speed or 10.0
    }
end

function Enemy(speed)
    return {
        speed = speed or 5.0
    }
end

function Bullet(damage)
    return {
        damage = damage or 1
    }
end

function Life(amount)
    return {
        amount = amount or 100,
        max = amount or 100
    }
end

function Tag(tags)
    return {
        tags = tags or {}
    }
end

function Bonus(duration)
    return {
        duration = duration or 5.0,
        type = "shootSpeed"
    }
end

function PowerUp(duration, originalCooldown)
    return {
        timeRemaining = duration or 5.0,
        originalCooldown = originalCooldown or 0.2
    }
end

function Score(value)
    return {
        value = value or 0
    }
end
