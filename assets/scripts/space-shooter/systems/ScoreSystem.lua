-- ============================================================================
-- ScoreSystem.lua - Score Display for Rendering Instances
-- ============================================================================
-- Updates score text UI only on instances with rendering capability.
-- Score logic itself is managed by authority (Server/Solo).
-- ============================================================================

local ScoreSystem = {}


ScoreSystem.lastScore = 0

-- Global variable to persist score across levels
CurrentScore = CurrentScore or 0

function ScoreSystem.init()
    print("[ScoreSystem] Initialized (hasRendering: " .. tostring(ECS.capabilities.hasRendering) .. ")")
end

function ScoreSystem.update(dt)
    if ECS.isPaused then return end

    -- Only update text rendering on instances with rendering capability
    if not ECS.capabilities.hasRendering then return end

    local textEntities = ECS.getEntitiesWith({"Text"})
    if #textEntities == 0 then return end
    local textEntity = textEntities[1]

    local textComp = ECS.getComponent(textEntity, "Text")

    -- Get player's score
    local playerEntities = ECS.getEntitiesWith({"Player", "Score"})
    if #playerEntities == 0 then return end
    local playerScoreComp = ECS.getComponent(playerEntities[1], "Score")

    if playerScoreComp.value ~= ScoreSystem.lastScore then
        ScoreSystem.lastScore = playerScoreComp.value
        textComp.text = "Score: " .. playerScoreComp.value
        ECS.addComponent(textEntity, "Text", textComp)
        CurrentScore = playerScoreComp.value
        print("[SCORE] " .. CurrentScore)

    end

end


function ScoreSystem.adjustToScreenSize(width, height)
    local textEntities = ECS.getEntitiesWith({"Text"})
    if #textEntities == 0 then return end
    local textId = textEntities[1]
    local scoreEntities = ECS.getEntitiesWith({"Score", "Transform", "Text"})
    if #scoreEntities == 0 then return end
    
    local scoreId = scoreEntities[1]
    local t = ECS.getComponent(scoreId, "Transform")
    -- Position at 5% from top-left corner
    t.x = width * 0.03
    t.y = height * 0.03
    ECS.addComponent(scoreId, "Transform", t)
    
    local textComp = ECS.getComponent(scoreId, "Text")
    -- Scale font size to ~8% of screen height
    textComp.size = math.floor(height * 0.08)
    ECS.addComponent(scoreId, "Text", textComp)
    
    print("[ScoreSystem] Score adjusted to screen size: " .. width .. "x" .. height)
end

ECS.registerSystem(ScoreSystem)
