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
    
    local scoreEntities = ECS.getEntitiesWith({"Score", "Text"})
    if #scoreEntities == 0 then return end

    local scoreEntity = scoreEntities[1]
    local scoreComp = ECS.getComponent(scoreEntity, "Score")
    local textComp = ECS.getComponent(scoreEntity, "Text")

    if scoreComp.value ~= ScoreSystem.lastScore then
        ScoreSystem.lastScore = scoreComp.value
        textComp.text = "Score: " .. scoreComp.value
        CurrentScore = scoreComp.value
    end

end

ECS.registerSystem(ScoreSystem)
