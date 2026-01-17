local config = {
    player = {
        speed = 10.0,
        life = 3,
        scale = 0.7,
        weaponCooldown = 0.2,
        collider = { type = "Box", size = {1, 1, 1} },
        color = { r = 0.0, g = 1.0, b = 0.0 },
        boundaries = {
            minX = -15.0,
            maxX = 15.0,
            minY = -10.0,
            maxY = 10.0
        }
    },
    bullet = {
        damage = 10,
        speed = 20.0,
        collider = { type = "Sphere", size = {0.2} },
        life = 1,
    },
    enemy = {
        baseSpawnInterval = 2.0,
        baseSpeed = 5.0,
        minSpawnInterval = 0.8,
        maxSpeed = 15.0,
        life = 1,
        scale = 1.9,
        collider = { type = "Box", size = {1, 1, 1} },
    },
    score = {
        kill = 10,
        escapePenalty = 20,
        levelThresholds = {700, 1900, 3350, 4500, 6000},
    }
}

return config
