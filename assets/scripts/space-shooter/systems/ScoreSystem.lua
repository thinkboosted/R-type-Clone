local ScoreSystem = {}

ScoreSystem.lastScore = 0
ScoreSystem.levelChanged = false

function ScoreSystem.init()
    print("[ScoreSystem] Initialized")
end

function ScoreSystem.update(dt)
    local scoreEntities = ECS.getEntitiesWith({"Score", "Text"})
    if #scoreEntities == 0 then return end

    local scoreEntity = scoreEntities[1]
    local scoreComp = ECS.getComponent(scoreEntity, "Score")
    local textComp = ECS.getComponent(scoreEntity, "Text")

    if scoreComp.value ~= ScoreSystem.lastScore then
        ScoreSystem.lastScore = scoreComp.value
        textComp.text = "Score: " .. scoreComp.value
    end

    if scoreComp.value >= 100 and not ScoreSystem.levelChanged then
        ScoreSystem.levelChanged = true
        print("Level Complete! Switching to Level 2...")
        ECS.saveState("space-shooter-save-level-1-complete")
        ECS.removeEntities()
        ECS.removeSystems()
        dofile("assets/scripts/space-shooter/levels/Level-2.lua")
    end
end

ECS.registerSystem(ScoreSystem)
