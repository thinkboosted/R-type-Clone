local PhysicSystem = {}

PhysicSystem.initializedEntities = {}

function PhysicSystem.init()
    print("[PhysicSystem] Initialized")
end

function PhysicSystem.update(dt)
    local entities = ECS.getEntitiesWith({"Transform", "Physic", "Collider"})

    for _, id in ipairs(entities) do
        local transform = ECS.getComponent(id, "Transform")
        local physic = ECS.getComponent(id, "Physic")
        local collider = ECS.getComponent(id, "Collider")

        if not PhysicSystem.initializedEntities[id] then

            local params = ""
            if collider.type == "Box" then
                params = collider.size[1] .. "," .. collider.size[2] .. "," .. collider.size[3]
            elseif collider.type == "Sphere" then
                params = tostring(collider.size)
            end

            local msg = "CreateBody:" .. id .. ":" .. collider.type .. ":" .. params .. "," .. physic.mass .. ";"
            ECS.sendMessage("PhysicCommand", msg)

            -- Set initial transform
            local tMsg = "SetTransform:" .. id .. ":" .. transform.x .. "," .. transform.y .. "," .. transform.z .. ":" .. transform.rx .. "," .. transform.ry .. "," .. transform.rz .. ";"
            ECS.sendMessage("PhysicCommand", tMsg)

            PhysicSystem.initializedEntities[id] = true
        end

        local vMsg = "SetLinearVelocity:" .. id .. ":" .. physic.vx .. "," .. physic.vy .. "," .. physic.vz .. ";"
        ECS.sendMessage("PhysicCommand", vMsg)
    end
end

function PhysicSystem.onCollision(id1, id2)
    print("[PhysicSystem] Collision detected between " .. id1 .. " and " .. id2)
end

function PhysicSystem.onEntityUpdated(id, x, y, z, rx, ry, rz)
    local transform = ECS.getComponent(id, "Transform")
    if transform then
        transform.x = x
        transform.y = y
        transform.z = z
        transform.rx = rx
        transform.ry = ry
        transform.rz = rz
    end
end

ECS.registerSystem(PhysicSystem)
