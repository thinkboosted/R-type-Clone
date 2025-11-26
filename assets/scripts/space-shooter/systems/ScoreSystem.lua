local ScoreSystem = {}

ScoreSystem.lastScore = 0

function ScoreSystem.init()
    print("[ScoreSystem] Initialized")
    print("Score: 0")
end

function ScoreSystem.update(dt)
    local scoreEntities = ECS.getEntitiesWith({"Score", "Text"})
    if #scoreEntities == 0 then return end
    
    local scoreEntity = scoreEntities[1]
    local scoreComp = ECS.getComponent(scoreEntity, "Score")
    local textComp = ECS.getComponent(scoreEntity, "Text")
    
    if scoreComp.value ~= ScoreSystem.lastScore then
        print("Score: " .. scoreComp.value)
        ScoreSystem.lastScore = scoreComp.value
        textComp.text = "Score: " .. scoreComp.value
    end
end

ECS.registerSystem(ScoreSystem)
