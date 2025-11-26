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

function Camera(fov)
    return {
        fov = fov or 90.0,
        isActive = true
    }
end

function Color(r, g, b)
    return { r = r, g = g, b = b }
end
