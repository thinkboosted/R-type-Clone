local WinSystem = {}

WinSystem.levelChanged = false

function WinSystem.init()
    print("[WinSystem] Initialized")
end

function WinSystem.update(dt)
    -- Only run on rendering instances
    if not ECS.capabilities.hasRendering then return end

    if WinSystem.levelChanged then return end

    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities == 0 then return end

    local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
    if scoreComp.value >= 100 then
        WinSystem.levelChanged = true
        print("Level Complete! Switching to Level " .. CurrentLevel + 1 .. "...")
        ECS.saveState("space-shooter-save-level-" .. CurrentLevel .. "-complete")
        -- Save score before removing entities
        CurrentScore = scoreComp.value
        -- ECS.removeEntities()
        -- ECS.removeSystems()
        -- Remove only level-specific entities
        local toRemove = {}
        for _, tag in ipairs({"Enemy", "Bullet", "Background"}) do
            local entities = ECS.getEntitiesWith({tag})
            for _, id in ipairs(entities) do
                table.insert(toRemove, id)
            end
        end
        for _, id in ipairs(toRemove) do
            ECS.destroyEntity(id)
        end
        -- Re-register systems (if needed, e.g., dofile("assets/scripts/space-shooter/GameLoop.lua"))
        dofile("assets/scripts/space-shooter/levels/Level-" .. CurrentLevel + 1 .. ".lua")
    end
end


ECS.registerSystem(WinSystem)
