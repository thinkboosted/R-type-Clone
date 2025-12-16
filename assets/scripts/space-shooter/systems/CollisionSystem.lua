local CollisionSystem = {}

CollisionSystem.initializedEntities = {}

function CollisionSystem.init()
    print("[CollisionSystem] Initialized")
end

function CollisionSystem.update(dt)
    local entities = ECS.getEntitiesWith({"Transform", "Collider"})

    for _, id in ipairs(entities) do
        local transform = ECS.getComponent(id, "Transform")
        local collider = ECS.getComponent(id, "Collider")
        local physic = ECS.getComponent(id, "Physic")

        if not CollisionSystem.initializedEntities[id] then
            local params = ""
            if collider.type == "Box" then
                params = collider.size[1] .. "," .. collider.size[2] .. "," .. collider.size[3]
            elseif collider.type == "Sphere" then
                params = tostring(collider.size[1])
            end

            local mass = 0.0
            local friction = 0.5
            local fixedRotation = false
            if physic then
                mass = physic.mass
                friction = physic.friction
                fixedRotation = physic.fixedRotation
            end

            local msg = "CreateBody:" .. id .. ":" .. collider.type .. ":" .. params .. "," .. mass .. "," .. friction .. ";"
            ECS.sendMessage("PhysicCommand", msg)

            if fixedRotation then
                ECS.sendMessage("PhysicCommand", "SetAngularFactor:" .. id .. ":0,0,0;")
            end

            local tMsg = "SetTransform:" .. id .. ":" .. transform.x .. "," .. transform.y .. "," .. transform.z .. ":" .. transform.rx .. "," .. transform.ry .. "," .. transform.rz .. ";"
            ECS.sendMessage("PhysicCommand", tMsg)

            CollisionSystem.initializedEntities[id] = true
        end
    end
end

function CollisionSystem.hasTag(id, tag)
    local tagComp = ECS.getComponent(id, "Tag")
    if tagComp and tagComp.tags then
        for _, t in ipairs(tagComp.tags) do
            if t == tag then
                return true
            end
        end
    end
    return false
end

function CollisionSystem.onCollision(id1, id2)
    -- Check Player vs Enemy
    if CollisionSystem.hasTag(id1, "Player") and CollisionSystem.hasTag(id2, "Enemy") then
        CollisionSystem.handlePlayerEnemy(id1, id2)
    elseif CollisionSystem.hasTag(id2, "Player") and CollisionSystem.hasTag(id1, "Enemy") then
        CollisionSystem.handlePlayerEnemy(id2, id1)
    end

    -- Check Enemy vs Bullet
    if CollisionSystem.hasTag(id1, "Enemy") and CollisionSystem.hasTag(id2, "Bullet") then
        CollisionSystem.handleEnemyBullet(id1, id2)
    elseif CollisionSystem.hasTag(id2, "Enemy") and CollisionSystem.hasTag(id1, "Bullet") then
        CollisionSystem.handleEnemyBullet(id2, id1)
    end

    -- Check Player vs Bonus
    if CollisionSystem.hasTag(id1, "Player") and CollisionSystem.hasTag(id2, "Bonus") then
        CollisionSystem.handlePlayerBonus(id1, id2)
    elseif CollisionSystem.hasTag(id2, "Player") and CollisionSystem.hasTag(id1, "Bonus") then
        CollisionSystem.handlePlayerBonus(id2, id1)
    end
end

function CollisionSystem.handlePlayerEnemy(playerId, enemyId)
    local life = ECS.getComponent(playerId, "Life")
    if life then
        life.amount = 0
    end
end

function CollisionSystem.handleEnemyBullet(enemyId, bulletId)
    local life = ECS.getComponent(enemyId, "Life")
    if life then
        life.amount = 0
    end

    -- Increase score
    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities > 0 then
        local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
        scoreComp.value = scoreComp.value + 10
    end

    local bLife = ECS.getComponent(bulletId, "Life")
    if bLife then
        bLife.amount = 0
    end
end

function CollisionSystem.handlePlayerBonus(playerId, bonusId)
    local bonus = ECS.getComponent(bonusId, "Bonus")
    if bonus then
        ECS.addComponent(playerId, "PowerUp", PowerUp(bonus.duration, 0.2))

        local bonusLife = ECS.getComponent(bonusId, "Life")
        if bonusLife then
            bonusLife.amount = 0
        end
    end
end

ECS.registerSystem(CollisionSystem)
