local PhysicSystem = {}

PhysicSystem.initializedEntities = {}
PhysicSystem.previousStates = {}

function PhysicSystem.init()
    print("[PhysicSystem] Initialized")
end

function PhysicSystem.update(dt)
    local entities = ECS.getEntitiesWith({"Transform", "Physic"})

    for _, id in ipairs(entities) do
        local transform = ECS.getComponent(id, "Transform")
        local physic = ECS.getComponent(id, "Physic")
        local collider = ECS.getComponent(id, "Collider")

        -- If entity has a collider, we assume it's managed by the physics engine
        if collider then
            if not PhysicSystem.previousStates[id] then
                PhysicSystem.previousStates[id] = { mass = physic.mass, friction = physic.friction }
            else
                local prevState = PhysicSystem.previousStates[id]
                if prevState.mass ~= physic.mass then
                    ECS.sendMessage("PhysicCommand", "SetMass:" .. id .. ":" .. physic.mass .. ";")
                    prevState.mass = physic.mass
                end
                if prevState.friction ~= physic.friction then
                    ECS.sendMessage("PhysicCommand", "SetFriction:" .. id .. ":" .. physic.friction .. ";")
                    prevState.friction = physic.friction
                end
            end

            -- Sync velocity to physics engine
            if physic.mass > 0 then
                local vMsg = "SetLinearVelocity:" .. id .. ":" .. physic.vx .. "," .. physic.vy .. "," .. physic.vz .. ";"
                ECS.sendMessage("PhysicCommand", vMsg)

                local vaMsg = "SetAngularVelocity:" .. id .. ":" .. physic.vax .. "," .. physic.vay .. "," .. physic.vaz .. ";"
                ECS.sendMessage("PhysicCommand", vaMsg)

                if not physic.useGravity then
                    -- Apply anti-gravity force: F = m * g_up. g is -30, so g_up is 30.
                    local forceY = physic.mass * 30.0
                    ECS.sendMessage("PhysicCommand", "ApplyForce:" .. id .. ":0," .. forceY .. ",0;")
                end
            end
        else
            -- Simple manual physics if no collider/physics engine body
            if physic.mass > 0 then
                if physic.useGravity then
                    physic.vy = physic.vy - 9.81 * dt -- Manual gravity if not using engine
                end
            end

            transform.x = transform.x + physic.vx * dt
            transform.y = transform.y + physic.vy * dt
            transform.z = transform.z + physic.vz * dt

            transform.rx = transform.rx + physic.vax * dt
            transform.ry = transform.ry + physic.vay * dt
            transform.rz = transform.rz + physic.vaz * dt
        end
    end
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
