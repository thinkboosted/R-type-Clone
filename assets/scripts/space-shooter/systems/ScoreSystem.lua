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

    -- Don't display score in multiplayer mode
    if ECS.gameMode == "MULTI_CLIENT" then return end

    local textEntities = ECS.getEntitiesWith({"Text"})
    if #textEntities == 0 then return end
    local textEntity = textEntities[1]

    local textComp = ECS.getComponent(textEntity, "Text")

    -- Get score based on mode
    local scoreValue = 0
    if ECS.gameMode == "SOLO" then
        -- In solo, display player's individual score
        local playerEntities = ECS.getEntitiesWith({"Player", "Score"})
        if #playerEntities > 0 then
            local playerScoreComp = ECS.getComponent(playerEntities[1], "Score")
            scoreValue = playerScoreComp.value
        end
    else
        -- In other modes, use global score
        local scoreEntities = ECS.getEntitiesWith({"Score"})
        if #scoreEntities > 0 then
            local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
            scoreValue = scoreComp.value
        end
    end

    if scoreValue ~= ScoreSystem.lastScore then
        ScoreSystem.lastScore = scoreValue
        textComp.text = "Score: " .. scoreValue
        ECS.addComponent(textEntity, "Text", textComp)
        CurrentScore = scoreValue
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
