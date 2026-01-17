local PhysicsSystem = {}

function PhysicsSystem.init()
    print("[PhysicsSystem] Init (Hybrid Mode: Engine + Prediction)")
end

function PhysicsSystem.update(dt)
    -- Manual Update (Client Prediction) to ensure visual movement
    -- regardless of Engine feedback frequency.
    
    local entities = ECS.getEntitiesWith({"Physic", "Transform"})
    for _, id in ipairs(entities) do
        local ph = ECS.getComponent(id, "Physic")
        local t = ECS.getComponent(id, "Transform")
        
        -- If velocity is set (by InputSystem), move the transform
        if ph.vx ~= 0 or ph.vy ~= 0 or ph.vz ~= 0 then
            t.x = t.x + ph.vx * dt
            t.y = t.y + ph.vy * dt
            t.z = t.z + ph.vz * dt
            
            -- Simple Boundary Checks (Client Side Prediction)
            -- Paddle Limits
            if ECS.hasComponent(id, "Paddle") then
                if t.x < -20 then t.x = -20 end
                if t.x > 20 then t.x = 20 end
            end
            
            -- Ball Bounce (Simple Prediction)
            if ECS.hasComponent(id, "Ball") then
                if t.x < -24 or t.x > 24 then 
                    ph.vx = -ph.vx 
                    -- Sync bounce to engine
                    if ECS.setLinearVelocity then
                        ECS.setLinearVelocity(id, ph.vx, ph.vy, ph.vz)
                    end
                end
                if t.y > 24 then 
                    ph.vy = -ph.vy
                    if ECS.setLinearVelocity then
                        ECS.setLinearVelocity(id, ph.vx, ph.vy, ph.vz)
                    end
                end
            end
            
            -- Update Component -> Triggers syncToRenderer -> Visual Update
            ECS.updateComponent(id, "Transform", t)
            
            -- Keep Physic component updated for next frame logic
            ECS.updateComponent(id, "Physic", ph)
        end
    end
end

-- Callback called by C++ Engine when Bullet detects collision
function PhysicsSystem.onCollision(id1, id2)
    local isBall1 = ECS.hasComponent(id1, "Ball")
    local isBall2 = ECS.hasComponent(id2, "Ball")
    
    if isBall1 or isBall2 then
        local ballId = isBall1 and id1 or id2
        local otherId = isBall1 and id2 or id1
        
        -- Play sound regardless
        ECS.playSound("effects/laser.wav")
        
        -- Simple Bounce Logic on Collision (Client Side)
        local ph = ECS.getComponent(ballId, "Physic")
        if ph then
            -- Invert Y for simple bounce (Physics Engine will do better, but this gives instant feedback)
            -- Ideally we'd use normals, but for breakout, Y inversion is often enough for paddle/bricks
            ph.vy = -ph.vy
            
            -- Add some randomness or X influence based on paddle hit?
            -- For now keep simple
            ECS.updateComponent(ballId, "Physic", ph)
            if ECS.setLinearVelocity then
                ECS.setLinearVelocity(ballId, ph.vx, ph.vy, ph.vz)
            end
        end

        if ECS.hasComponent(otherId, "Brick") then
            print("[PhysicsSystem] Ball hit Brick " .. otherId)
            ECS.destroyEntity(otherId)
            ECS.playSound("effects/explosion.wav")
        end
    end
end

-- Sync with Engine if it sends updates (Correction)
function PhysicsSystem.onEntityUpdated(id, x, y, z, rx, ry, rz)
    local t = ECS.getComponent(id, "Transform")
    if t then
        -- Interpolate or snap? For now snap.
        t.x = x
        t.y = y
        t.z = z
        ECS.updateComponent(id, "Transform", t)
    end
end

ECS.registerSystem(PhysicsSystem)