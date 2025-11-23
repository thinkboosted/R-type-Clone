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

function Collider(type, size)
    return {
        type = type or "Box",
        size = size or {1, 1, 1}
    }
end

function Physic(mass, friction)
    return {
        mass = mass or 1.0,
        friction = friction or 0.5,
        vx = 0, vy = 0, vz = 0,
        ax = 0, ay = 0, az = 0
    }
end

function Camera(fov)
    return {
        fov = fov or 90.0,
        isActive = true
    }
end

function Player(speed)
    return {
        speed = speed or 5.0,
        inputX = 0,
        inputY = 0
    }
end
