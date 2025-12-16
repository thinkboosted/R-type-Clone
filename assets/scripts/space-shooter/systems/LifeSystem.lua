local LifeSystem = {}

function LifeSystem.init()
    print("[LifeSystem] Initialized")
end

function LifeSystem.update(dt)
    local entities = ECS.getEntitiesWith({"Life"})

    for _, id in ipairs(entities) do
        local life = ECS.getComponent(id, "Life")
        if life.amount <= 0 then
            -- Check if it's the player
            local tagComp = ECS.getComponent(id, "Tag")
            local isPlayer = false
            if tagComp and tagComp.tags then
                for _, t in ipairs(tagComp.tags) do
                    if t == "Player" then
                        isPlayer = true
                        break
                    end
                end
            end

            if isPlayer then
                -- Get final score
                local scoreEntities = ECS.getEntitiesWith({"Score"})
                local finalScore = 0
                if #scoreEntities > 0 then
                    local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
                    finalScore = scoreComp.value
                end
                print("GAME OVER: Player died!")
                print("Final Score: " .. finalScore)
            end

            ECS.destroyEntity(id)

        end
    end
end

ECS.registerSystem(LifeSystem)
