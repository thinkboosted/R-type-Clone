-- ============================================================================
-- BackgroundSystem.lua - Server-Authoritative Life Management
-- ============================================================================

local BackgroundSystem = {}

function BackgroundSystem.init()
    print("[BackgroundSystem] Initialized")
end

function BackgroundSystem.update(dt)
    -- Only run on rendering instances (Client/Solo)
    if not ECS.capabilities.hasRendering then return end

    local images = ECS.getEntitiesWith({"Background", "Transform"})

    for _, id in ipairs(images) do
        local bg = ECS.getComponent(id, "Background")
        local transform = ECS.getComponent(id, "Transform")

        -- Move background
        transform.x = transform.x + (bg.scrollSpeed * dt)

        -- Reset position if off-screen
        if transform.x <= bg.endX then
            transform.x = bg.resetX
        end
    end
end

ECS.registerSystem(BackgroundSystem)