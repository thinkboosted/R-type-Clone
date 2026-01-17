-- Component Definitions

function Transform(x, y, z, rx, ry, rz, sx, sy, sz)
    return {
        x = x or 0,
        y = y or 0,
        z = z or 0,
        rx = rx or 0,
        ry = ry or 0,
        rz = rz or 0,
        sx = sx or 1,
        sy = sy or 1,
        sz = sz or 1
    }
end

function Mesh(modelPath, texturePath)
    return {
        modelPath = modelPath,
        texturePath = texturePath
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

function Weapon(cooldown)
    return {
        cooldown = cooldown or 0.2,
        timeSinceLastShot = 0.0
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

function Animation(frameCount, frameDuration, isLooping, modelBase, textureBase)
    return {
        frameCount = frameCount or 1,
        frameDuration = frameDuration or 0.1,
        currentFrame = 1,
        elapsedTime = 0.0,
        isLooping = isLooping or true,
        isActive = true,
        modelBase = modelBase or "",
        textureBase = textureBase or "",
        frames = {},
        type = "sequence"
    }
end

function MovementPattern(patternType, amplitude, frequency, speed)
    return {
        patternType = patternType or "linear", -- "sine", "zigzag", "circle"
        amplitude = amplitude or 1.0,
        frequency = frequency or 1.0,
        speed = speed or 1.0,
        startX = 0,
        startY = 0,
        startZ = 0,
        time = 0.0,
        initialized = false
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

function Text(text, fontPath, fontSize, isScreenSpace)
    return {
        text = text or "",
        fontPath = fontPath or "assets/fonts/arial.ttf",
        fontSize = fontSize or 24,
        isScreenSpace = isScreenSpace or false
    }
end

function Follow(targetId, offsetX, offsetY, offsetZ)
    return {
        targetId = targetId,
        offsetX = offsetX or 0,
        offsetY = offsetY or 0,
        offsetZ = offsetZ or 0
    }
end

function NetworkId(id)
    return {
        id = id or 0
    }
end

function ParticleGenerator(offsetX, offsetY, offsetZ, dirX, dirY, dirZ, spread, speed, lifeTime, rate, size, r, g, b)
    return {
        offsetX = offsetX or 0,
        offsetY = offsetY or 0,
        offsetZ = offsetZ or 0,
        dirX = dirX or 0,
        dirY = dirY or 1,
        dirZ = dirZ or 0,
        spread = spread or 0.5,
        speed = speed or 1.0,
        lifeTime = lifeTime or 1.0,
        rate = rate or 10.0,
        size = size or 0.1,
        r = r or 1.0,
        g = g or 1.0,
        b = b or 1.0
    }
end

-- ============================================================================
-- SERVER-AUTHORITATIVE ARCHITECTURE COMPONENTS
-- ============================================================================
-- These components are required for the unified Client-Server architecture
-- as defined in docs/examples/

-- NetworkIdentity - Identifies network entities with ownership
function NetworkIdentity(uuid, ownerId, isLocalPlayer)
    return {
        uuid = uuid or "",
        ownerId = ownerId or -1,              -- -1 = server, >= 0 = client
        isLocalPlayer = isLocalPlayer or false,
        lastSyncTime = 0,
        interpolate = true
    }
end

-- InputBuffer - Buffer inputs for prediction/reconciliation (Client only)
function InputBuffer(maxHistory)
    return {
        sequence = 0,
        commands = {},
        maxHistory = maxHistory or 60,
        lastProcessedSeq = 0
    }
end

-- ServerAuthority - Marks entities with server authority
function ServerAuthority()
    return {
        canSimulate = true,
        broadcastRate = 0.05,
        timeSinceLastBroadcast = 0
    }
end

-- ClientPredicted - Marks entities with client-side prediction
function ClientPredicted()
    return {
        enabled = true,
        reconcile = true,
        snapThreshold = 2.0
    }
end

-- InputState - Unified input state component (replaces direct key checks)
function InputState()
    return {
        up = false,
        down = false,
        left = false,
        right = false,
        shoot = false
    }
end

-- GameState - Server-Authoritative game state management
-- Replaces MenuSystem global variables with ECS-based state
function GameState(state, lastScore)
    return {
        state = state or "MENU",              -- MENU, PLAYING, GAMEOVER, ERROR
        lastScore = lastScore or 0,
        connectionTimeout = 0,
        assignmentHandled = false,
        -- Server-side transition tracking
        transitionRequested = false,
        nextState = nil
    }
end

-- For Paralax effect
function Background(scrollSpeed, resetX, endX)
    return {
        scrollSpeed = scrollSpeed or -2.0,
        resetX = resetX or 60.0,
        endX = endX or -60.0
    }
end
