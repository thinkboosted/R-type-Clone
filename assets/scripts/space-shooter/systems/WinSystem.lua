local config = dofile("assets/scripts/space-shooter/config.lua")
local WinSystem = {}

WinSystem.levelChanged = false
WinSystem.bossSpawnRequested = {}  -- Track which levels have requested boss spawns

local function getMaxLevel()
    local thresholds = config.score.levelThresholds or {}
    return (#thresholds + 1)
end

local function getBossThresholdForLevel(level)
    local thresholds = config.score.levelThresholds or {}
    if thresholds[level] then
        return thresholds[level]
    end

    -- Final level fallback: require more score than previous threshold.
    local last = thresholds[#thresholds] or 700
    return last + 700
end

local function advanceToLevel(nextLevel, currentScore)
    local maxLevel = getMaxLevel()
    if nextLevel > maxLevel then
        ECS.sendMessage("GAME_WON", "all_levels_cleared")
        if ECS.capabilities.hasNetworkSync then
            ECS.broadcastNetworkMessage("GAME_WON", "all_levels_cleared")
        end
        print("[WinSystem] All levels cleared at score " .. tostring(currentScore))
        return
    end

    -- Destroy existing score text entities to avoid duplicates
    local existingScoreEntities = ECS.getEntitiesWith({"Score", "Transform", "Text"})
    for _, id in ipairs(existingScoreEntities) do
        ECS.destroyEntity(id)
    end

    local toRemove = {}
    for _, tag in ipairs({"Background", "Text"}) do
        local entities = ECS.getEntitiesWith({tag})
        for _, id in ipairs(entities) do
            table.insert(toRemove, id)
        end
    end
    for _, id in ipairs(toRemove) do
        ECS.destroyEntity(id)
    end

    _G.CurrentLevel = nextLevel

    local file = io.open("current_level.txt", "w")
    if file then
        file:write(tostring(nextLevel))
        file:close()
    end

    dofile("assets/scripts/space-shooter/levels/Level-" .. nextLevel .. ".lua")

    -- Reset level-local boss state and enemy pacing.
    _G.LevelBossActive = false
    WinSystem.bossSpawnRequested[nextLevel] = false  -- Clear flag for new level
    ECS.sendMessage("RESET_BOSS_STATE", "")
    if EnemySystem then
        EnemySystem.resetDifficulty()
    end

    if ECS.capabilities.hasNetworkSync then
        ECS.broadcastNetworkMessage("LEVEL_CHANGE", tostring(nextLevel))
    end

    print("[WinSystem] Advanced to level " .. tostring(nextLevel) .. " at score " .. tostring(currentScore))
end

function WinSystem.init()
    print("[WinSystem] Initialized")

    if not _G.LevelBossDefeated then
        _G.LevelBossDefeated = {}
    end
    _G.LevelBossActive = false
    _G.PendingLevelAdvance = nil
    WinSystem.bossSpawnRequested = {}

    ECS.subscribe("BOSS_DEFEATED", function(msg)
        if not ECS.capabilities.hasAuthority then return end

        local defeatedLevel = tonumber(msg) or (_G.CurrentLevel or 1)
        _G.LevelBossDefeated[defeatedLevel] = true
        _G.LevelBossActive = false
        _G.PendingLevelAdvance = defeatedLevel
        WinSystem.bossSpawnRequested[defeatedLevel] = false  -- Reset for next cycle
        _G.PendingLevelAdvance = defeatedLevel
    end)
end

function WinSystem.update(dt)
    if not ECS.capabilities.hasAuthority then return end
    if not ECS.isGameRunning then return end

    local currentLevel = _G.CurrentLevel or 1

    if _G.PendingLevelAdvance == currentLevel and not _G.AdvancingLevel then
        _G.AdvancingLevel = true

        local scoreEntities = ECS.getEntitiesWith({"Score"})
        local scoreValue = 0
        if #scoreEntities > 0 then
            local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
            scoreValue = scoreComp and scoreComp.value or 0
        end

        advanceToLevel(currentLevel + 1, scoreValue)
        _G.PendingLevelAdvance = nil
        _G.AdvancingLevel = false
        return
    end

    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities == 0 then return end

    local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
    local currentScore = scoreComp and scoreComp.value or 0

    if not _G.LevelBossDefeated then
        _G.LevelBossDefeated = {}
    end

    if _G.LevelBossDefeated[currentLevel] then
        return
    end

    local threshold = getBossThresholdForLevel(currentLevel)
    if currentScore >= threshold and not _G.LevelBossActive then
        -- Only send the spawn request ONCE per level
        if not WinSystem.bossSpawnRequested[currentLevel] then
            _G.LevelBossActive = true
            WinSystem.bossSpawnRequested[currentLevel] = true

            local clearList = ECS.getEntitiesWith({"Enemy"})
            for _, id in ipairs(clearList) do
                ECS.broadcastNetworkMessage("ENTITY_DESTROY", tostring(id))
                ECS.sendMessage("PhysicCommand", "DestroyBody:" .. id .. ";")
                ECS.destroyEntity(id)
            end
            clearList = ECS.getEntitiesWith({"Bullet"})
            for _, id in ipairs(clearList) do
                ECS.broadcastNetworkMessage("ENTITY_DESTROY", tostring(id))
                ECS.sendMessage("PhysicCommand", "DestroyBody:" .. id .. ";")
                ECS.destroyEntity(id)
            end

            ECS.sendMessage("BOSS_SPAWN_REQUEST", tostring(currentLevel))
            if ECS.capabilities.hasNetworkSync then
                ECS.broadcastNetworkMessage("BOSS_SPAWN_REQUEST", tostring(currentLevel))
            end
            print("[WinSystem] Boss spawn threshold reached for level " .. tostring(currentLevel))
        end
    end
end

ECS.registerSystem(WinSystem)
