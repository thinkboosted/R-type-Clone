if _G.ScoreSystem and _G.ScoreSystem.__registered then
    return _G.ScoreSystem
end

local ScoreSystem = _G.ScoreSystem or {}

ScoreSystem.lastScore = ScoreSystem.lastScore or -1
ScoreSystem.uiTextId = ScoreSystem.uiTextId or nil

CurrentScore = CurrentScore or 0

local function ensureScoreUI()
    if ScoreSystem.uiTextId then
        return
    end
    ScoreSystem.uiTextId = ECS.createUIText("Score: 0", 24, 24, 28, 1.0, 1.0, 1.0, 70)
end

local function readScoreValue()
    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities > 0 then
        local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
        if scoreComp and scoreComp.value then
            return scoreComp.value
        end
    end

    local playerEntities = ECS.getEntitiesWith({"Player", "Score"})
    if #playerEntities > 0 then
        local scoreComp = ECS.getComponent(playerEntities[1], "Score")
        if scoreComp and scoreComp.value then
            return scoreComp.value
        end
    end

    return CurrentScore or 0
end

function ScoreSystem.init()
    print("[ScoreSystem] Initialized (hasRendering: " .. tostring(ECS.capabilities.hasRendering) .. ")")
end

function ScoreSystem.update(dt)
    if ECS.isPaused then return end
    if not ECS.capabilities.hasRendering then return end

    ensureScoreUI()

    local scoreValue = readScoreValue()
    if scoreValue ~= ScoreSystem.lastScore then
        ScoreSystem.lastScore = scoreValue
        CurrentScore = scoreValue
        ECS.setUIText(ScoreSystem.uiTextId, "Score: " .. tostring(scoreValue))
    end
end

function ScoreSystem.adjustToScreenSize(width, height)
    if not ECS.capabilities.hasRendering then return end
    ensureScoreUI()

    local x = math.floor(width * 0.03)
    local y = math.floor(height * 0.03)
    ECS.setUIPosition(ScoreSystem.uiTextId, x, y)
end

if not ScoreSystem.__registered then
    ECS.registerSystem(ScoreSystem)
    ScoreSystem.__registered = true
end

_G.ScoreSystem = ScoreSystem
return ScoreSystem
