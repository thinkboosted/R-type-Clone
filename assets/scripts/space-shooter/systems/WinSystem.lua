local config = dofile("assets/scripts/space-shooter/config.lua")
local WinSystem = {}

WinSystem.levelChanged = false

function WinSystem.init()
    print("[WinSystem] Initialized")
end

function WinSystem.checkAndAdvanceLevel(currentScore)
    local thresholds = config.score.levelThresholds or {}
    local targetLevel = 1
    for i, thresh in ipairs(thresholds) do
        if currentScore >= thresh then
            targetLevel = i + 1
        else
            break
        end
    end

    if targetLevel > (_G.CurrentLevel or 1) then
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


        _G.CurrentLevel = targetLevel
        -- Write new level to file
        local file = io.open("current_level.txt", "w")
        if file then
            file:write(tostring(targetLevel))
            file:close()
        end
        -- Load new level
        dofile("assets/scripts/space-shooter/levels/Level-" .. targetLevel .. ".lua")
        -- Reset enemy difficulty
        if EnemySystem then
            EnemySystem.resetDifficulty()
        end
        -- Broadcast level change in multiplayer
        if ECS.capabilities.hasNetworkSync then
            ECS.broadcastNetworkMessage("LEVEL_CHANGE", tostring(targetLevel))
        end
        print("[WinSystem] Advanced to level " .. targetLevel .. " at score " .. currentScore)
    end
end

function WinSystem.update(dt)
    -- Only run on rendering instances
    if not ECS.capabilities.hasRendering then return end

    if WinSystem.levelChanged then return end

    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities == 0 then return end

    local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
    -- Use threshold-based advancement instead of hardcoded 100
    WinSystem.checkAndAdvanceLevel(scoreComp.value)
end

ECS.registerSystem(WinSystem)