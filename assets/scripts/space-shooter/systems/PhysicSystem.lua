local PhysicSystem = {}

function PhysicSystem.init()
    print("[PhysicSystem] Initialized")
    -- Note: EntityUpdated messages are handled by the C++ LuaECSManager
    -- which calls PhysicSystem.onEntityUpdated() directly for each update.
    -- No explicit subscription needed here.
end

function PhysicSystem.update(dt)
    -- GUARD : Si je ne suis pas le serveur (Local ou Distant), je ne touche pas à la physique.
    if not ECS.capabilities.hasAuthority then return end

    -- Stop updates if game is paused
    if ECS.isPaused then
        -- CRITICAL FIX: Force stop all entities when paused
        local entities = ECS.getEntitiesWith({"Physic"})
        for _, id in ipairs(entities) do
             -- Send 0 velocity to stop physics engine simulation
            local msg = string.format("SetLinearVelocity:%s:0,0,0;", id)
            ECS.sendMessage("PhysicCommand", msg)
        end
        return 
    end

    local entities = ECS.getEntitiesWith({"Physic"})

    for _, id in ipairs(entities) do
        local physic = ECS.getComponent(id, "Physic")
        -- Always send velocity to ensure we can stop the ship (send 0,0,0)
        local msg = string.format("SetLinearVelocity:%s:%f,%f,0;", id, physic.vx, physic.vy)
        ECS.sendMessage("PhysicCommand", msg)
    end
end

-- ============================================================================
-- CALLBACK: Physics Engine → Lua Transform Sync
-- ============================================================================
-- Called by C++ LuaECSManager after receiving EntityUpdated messages from Bullet.
-- The C++ code parses the message and invokes this with individual parameters.
-- Receives updated positions/rotations from physics simulation.
-- This is the "Missing Link" that syncs physics state back to rendering.
function PhysicSystem.onEntityUpdated(id, x, y, z, rx, ry, rz)
    -- Verify the entity has a Transform component
    local transform = ECS.getComponent(id, "Transform")
    if not transform then return end
    
    -- Update Transform with physics-calculated position and rotation
    -- print("[PhysicSystem] Updated Entity " .. id .. " to (" .. x .. ", " .. y .. ", " .. z .. ")")
    -- local transform = ECS.getComponent(id, "Transform") -- Removed redundant get
    transform.x = x
    transform.y = y
    transform.z = z
    transform.rx = rx
    transform.ry = ry
    transform.rz = rz
    
    -- ====================================================================
    -- CRITICAL: Force Visual Sync - Notify Renderer Immediately
    -- ====================================================================
    -- Send SetPosition command to ensure renderer draws at correct position
    -- This bypasses any potential RenderSystem latency
    local renderMsg = string.format("SetPosition:%s,%f,%f,%f", id, x, y, z)
    ECS.sendMessage("RenderEntityCommand", renderMsg)

    local rotMsg = string.format("SetRotation:%s,%f,%f,%f", id, rx, ry, rz)
    ECS.sendMessage("RenderEntityCommand", rotMsg)
    
    -- print("[PhysicSystem] Forced Render Sync: " .. renderMsg)
end

-- Register system at end to avoid init recursion
ECS.registerSystem(PhysicSystem)
return PhysicSystem