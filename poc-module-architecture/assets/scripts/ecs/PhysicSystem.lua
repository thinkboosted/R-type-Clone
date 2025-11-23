local PhysicSystem = {}

PhysicSystem.initializedEntities = {}

function PhysicSystem.init()
    print("[PhysicSystem] Initialized")
end

function PhysicSystem.update(dt)
    local entities = ECS.getEntitiesWith({"Transform", "Physic"})

    for _, id in ipairs(entities) do
        local transform = ECS.getComponent(id, "Transform")
        local physic = ECS.getComponent(id, "Physic")
        local collider = ECS.getComponent(id, "Collider") -- Optional

        if collider then
            -- Managed by C++ Physics Engine (via CollisionSystem creation)
            if physic.mass > 0 then
                local vMsg = "SetLinearVelocity:" .. id .. ":" .. physic.vx .. "," .. physic.vy .. "," .. physic.vz .. ";"
                ECS.sendMessage("PhysicCommand", vMsg)

                local vaMsg = "SetAngularVelocity:" .. id .. ":" .. physic.vax .. "," .. physic.vay .. "," .. physic.vaz .. ";"
                ECS.sendMessage("PhysicCommand", vaMsg)
            end
        else
            -- Managed by Lua (Simple Physics)
            -- Apply Gravity
            if physic.mass > 0 then
                physic.vy = physic.vy - 9.81 * dt
            end

            -- Apply Velocity
            transform.x = transform.x + physic.vx * dt
            transform.y = transform.y + physic.vy * dt
            transform.z = transform.z + physic.vz * dt

            -- Apply Angular Velocity
            transform.rx = transform.rx + physic.vax * dt
            transform.ry = transform.ry + physic.vay * dt
            transform.rz = transform.rz + physic.vaz * dt
        end
    end
end

function PhysicSystem.onCollision(id1, id2)
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
