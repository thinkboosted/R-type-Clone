local GameLogicSystem = {}

function GameLogicSystem.init()
    print("[GameLogicSystem] Init")
end

function GameLogicSystem.update(dt)
    -- Check Win Condition
    local bricks = ECS.getEntitiesWith({"Brick"})
    if #bricks == 0 then
        -- Reset / Next Level
    end
end

ECS.registerSystem(GameLogicSystem)
