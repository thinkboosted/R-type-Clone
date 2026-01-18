-- Collision System for 3D Platformer
-- Initializes physics bodies for entities with colliders

CollisionSystem = {}

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
                params = tostring(collider.size)
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

ECS.registerSystem(CollisionSystem)
