-- Component Definitions for 3D Platformer

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

function Physic(mass, friction, fixedRotation)
    return {
        mass = mass or 1.0,
        friction = friction or 0.5,
        fixedRotation = fixedRotation or false,
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

function Player(speed, jumpForce)
    return {
        speed = speed or 10.0,
        jumpForce = jumpForce or 15.0,
        isGrounded = false,
        inputX = 0,
        inputZ = 0,
        jumpCooldown = 0.0,
        jumpCooldownTime = 0.5
    }
end

function Checkpoint(checkpointId)
    return {
        id = checkpointId or 0,
        activated = false
    }
end

function FinishLine(nextLevel)
    return {
        nextLevelScript = nextLevel or ""
    }
end

function DeathFloor()
    return {}
end

function Platform()
    return {}
end

function Color(r, g, b)
    return {
        r = r or 1.0,
        g = g or 1.0,
        b = b or 1.0
    }
end

function Light(color, intensity)
    return {
        r = color and color[1] or 1.0,
        g = color and color[2] or 1.0,
        b = color and color[3] or 1.0,
        intensity = intensity or 1.0
    }
end

function Text(content, fontSize, color)
    return {
        content = content or "",
        fontPath = "assets/fonts/arial.ttf",
        fontSize = fontSize or 48,
        isScreenSpace = true,
        r = color and color[1] or 1.0,
        g = color and color[2] or 1.0,
        b = color and color[3] or 1.0
    }
end
