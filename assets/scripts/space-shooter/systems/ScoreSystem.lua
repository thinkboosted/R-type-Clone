local ScoreSystem = {}

ScoreSystem.lastScore = 0

function ScoreSystem.init()
    print("[ScoreSystem] Initialized")
    print("Score: 0")
end

function ScoreSystem.update(dt)
    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities == 0 then return end
    
    local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
    if scoreComp.value ~= ScoreSystem.lastScore then
        print("Score: " .. scoreComp.value)
        ScoreSystem.lastScore = scoreComp.value
    end
end

ECS.registerSystem(ScoreSystem)
