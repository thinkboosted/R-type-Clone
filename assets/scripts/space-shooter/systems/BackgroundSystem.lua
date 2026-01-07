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
    -- Debug print to verify entities are found (only print if count > 0 to avoid spam if empty, or throttle)
    if #images > 0 and math.random() < 0.01 then
        print("[BackgroundSystem] Updating " .. #images .. " background entities. First ID: " .. images[1])
        local t = ECS.getComponent(images[1], "Transform")
        print("[BackgroundSystem] Entity " .. images[1] .. " X: " .. t.x)
    end

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