local CollisionSystem = {}

CollisionSystem.initializedEntities = {}

function CollisionSystem.init()
    print("[CollisionSystem] Initialized")
end

function CollisionSystem.update(dt)
    if not ECS.isServer() and not ECS.isLocal then return end
    if not ECS.isGameRunning then return end

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

function CollisionSystem.onCollision(id1, id2)
    -- Check Player vs Enemy
    local p1 = ECS.getComponent(id1, "Player")
    local e2 = ECS.getComponent(id2, "Enemy")
    local p2 = ECS.getComponent(id2, "Player")
    local e1 = ECS.getComponent(id1, "Enemy")

    if p1 and e2 then
        CollisionSystem.handlePlayerEnemy(id1, id2)
    elseif p2 and e1 then
        CollisionSystem.handlePlayerEnemy(id2, id1)
    end

    -- Check Enemy vs Bullet
    local en1 = ECS.getComponent(id1, "Enemy")
    local b2 = ECS.getComponent(id2, "Bullet")
    local en2 = ECS.getComponent(id2, "Enemy")
    local b1 = ECS.getComponent(id1, "Bullet")

    if en1 and b2 then
        CollisionSystem.handleEnemyBullet(id1, id2)
    elseif en2 and b1 then
        CollisionSystem.handleEnemyBullet(id2, id1)
    end

    -- Check Player vs Bonus
    local bonus1 = ECS.getComponent(id1, "Bonus")
    local bonus2 = ECS.getComponent(id2, "Bonus")

    if p1 and bonus2 then
        CollisionSystem.handlePlayerBonus(id1, id2)
    elseif p2 and bonus1 then
        CollisionSystem.handlePlayerBonus(id2, id1)
    end
end

function CollisionSystem.handlePlayerEnemy(playerId, enemyId)
    print("DEBUG: Collision Player " .. playerId .. " vs Enemy " .. enemyId)
    local life = ECS.getComponent(playerId, "Life")
    if life then
        if life.invulnerableTime and life.invulnerableTime > 0 then
            return
        end
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
