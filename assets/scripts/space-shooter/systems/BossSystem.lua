local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")

local BossSystem = {
    spawned = false,
    defeated = false,
    currentBossLevel = nil,
    spawnInProgress = false,  -- Prevent multiple concurrent spawn attempts
}

local function hasBossEntity()
    local bosses = ECS.getEntitiesWith({"Boss", "Life", "Transform"})
    return #bosses > 0, bosses
end

local function resetState()
    BossSystem.spawned = false
    BossSystem.defeated = false
    BossSystem.currentBossLevel = nil
    BossSystem.spawnInProgress = false
end

local function configureBossForLevel(bossId, level)
    local life = ECS.getComponent(bossId, "Life")
    local transform = ECS.getComponent(bossId, "Transform")
    local collider = ECS.getComponent(bossId, "Collider")
    local boss = ECS.getComponent(bossId, "Boss")

    local giantScale = 6.8 + (level - 1) * 1.6
    local hp = 320 + (level - 1) * 240

    if transform then
        transform.sx = giantScale
        transform.sy = giantScale
        transform.sz = giantScale
        ECS.addComponent(bossId, "Transform", transform)
    end

    if collider then
        collider.size = {giantScale * 0.75, giantScale * 0.75, giantScale * 0.75}
        ECS.addComponent(bossId, "Collider", collider)
    end

    if life then
        life.max = hp
        life.amount = hp
        ECS.addComponent(bossId, "Life", life)
    end

    if boss then
        boss.phase = 1
        boss.attackTimer = 0
        boss.moveTimer = 0
        ECS.addComponent(bossId, "Boss", boss)
    end
end

local function spawnBossForLevel(level)
    -- Prevent multiple spawn attempts in progress
    if BossSystem.spawnInProgress or BossSystem.spawned then
        return
    end

    local hasBoss = hasBossEntity()
    if hasBoss then
        BossSystem.spawned = true
        BossSystem.currentBossLevel = level
        return
    end

    BossSystem.spawnInProgress = true
    local bossId = Spawns.spawnBoss(16, 0, 0)
    configureBossForLevel(bossId, level)

    BossSystem.spawned = true
    BossSystem.defeated = false
    BossSystem.currentBossLevel = level
    BossSystem.spawnInProgress = false

    print("[BossSystem] Spawned giant boss for level " .. tostring(level))
end

local function updateBossPattern(bossId, dt)
    local boss = ECS.getComponent(bossId, "Boss")
    local transform = ECS.getComponent(bossId, "Transform")
    local phys = ECS.getComponent(bossId, "Physic")
    local life = ECS.getComponent(bossId, "Life")
    if not boss or not transform or not phys or not life then return end

    local level = BossSystem.currentBossLevel or (_G.CurrentLevel or 1)

    boss.attackTimer = (boss.attackTimer or 0) + dt
    boss.moveTimer = (boss.moveTimer or 0) + dt

    if life.amount <= life.max * 0.5 then
        boss.phase = 2
    end

    local phaseSpeed = (boss.phase == 1) and 1.5 or 2.4
    local verticalAmp = (boss.phase == 1) and (1.9 + level * 0.15) or (3.0 + level * 0.25)

    phys.vx = (transform.x > 10) and -1.6 or 0
    phys.vy = math.sin(boss.moveTimer * phaseSpeed) * verticalAmp

    local attackInterval = (boss.phase == 1) and math.max(0.8, 1.2 - level * 0.1) or math.max(0.45, 0.75 - level * 0.08)
    if boss.attackTimer >= attackInterval then
        boss.attackTimer = 0
        Spawns.spawnBullet(transform.x - 1.8, transform.y, transform.z, true)
        if boss.phase == 2 then
            Spawns.spawnBullet(transform.x - 1.8, transform.y + 0.8, transform.z, true)
            Spawns.spawnBullet(transform.x - 1.8, transform.y - 0.8, transform.z, true)
        end
    end

    ECS.addComponent(bossId, "Boss", boss)
    ECS.addComponent(bossId, "Physic", phys)
end

function BossSystem.init()
    print("[BossSystem] Initialized")

    ECS.subscribe("BOSS_SPAWN_REQUEST", function(msg)
        if not ECS.capabilities.hasAuthority then return end
        local level = tonumber(msg) or (_G.CurrentLevel or 1)
        spawnBossForLevel(level)
    end)

    ECS.subscribe("RESET_BOSS_STATE", function(_)
        resetState()
    end)
end

function BossSystem.update(dt)
    if ECS.isPaused then return end
    if not ECS.capabilities.hasAuthority then return end
    if not ECS.isGameRunning then return end

    local hasBoss, bosses = hasBossEntity()
    if not hasBoss then
        if BossSystem.spawned and not BossSystem.defeated and BossSystem.currentBossLevel then
            BossSystem.defeated = true
            local defeatedLevel = BossSystem.currentBossLevel
            _G.LevelBossActive = false

            if not _G.LevelBossDefeated then
                _G.LevelBossDefeated = {}
            end
            _G.LevelBossDefeated[defeatedLevel] = true
            _G.PendingLevelAdvance = defeatedLevel

            ECS.sendMessage("BOSS_DEFEATED", tostring(defeatedLevel))
            if ECS.capabilities.hasNetworkSync then
                ECS.broadcastNetworkMessage("BOSS_DEFEATED", tostring(defeatedLevel))
            end

            print("[BossSystem] Giant boss defeated on level " .. tostring(defeatedLevel))
        end
        return
    end

    for _, id in ipairs(bosses) do
        updateBossPattern(id, dt)
    end
end

ECS.registerSystem(BossSystem)
return BossSystem
