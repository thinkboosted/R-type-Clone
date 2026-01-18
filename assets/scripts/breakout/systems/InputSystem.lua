local InputSystem = {}

function InputSystem.init()
    print("[InputSystem] Init (Engine Physics Control)")
end

function InputSystem.update(dt)
    -- Quit Game
    if ECS.isKeyPressed("ESCAPE") then
        ECS.sendMessage("ExitApplication", "")
        return
    end

    -- Control Paddle
    local paddles = ECS.getEntitiesWith({"Paddle"})
    for _, id in ipairs(paddles) do
        local p = ECS.getComponent(id, "Paddle")
        
        local vx = 0
        if ECS.isKeyPressed("LEFT") or ECS.isKeyPressed("Q") then vx = -p.speed end
        if ECS.isKeyPressed("RIGHT") or ECS.isKeyPressed("D") then vx = p.speed end
        
        -- Throttling: Only send velocity if it changed
            if ECS.setLinearVelocity then
                ECS.setLinearVelocity(id, vx, 0, 0)
                p.lastVx = vx
                ECS.updateComponent(id, "Paddle", p)
            end
    end
    
    -- Launch Ball
    if ECS.isKeyPressed("SPACE") then
        local balls = ECS.getEntitiesWith({"Ball"})
        for _, id in ipairs(balls) do
            local b = ECS.getComponent(id, "Ball")
            
            if not b.active then
                b.active = true
                -- Send initial velocity to engine
                if ECS.setLinearVelocity then
                    ECS.setLinearVelocity(id, 5.0, b.speed, 0)
                end
                ECS.updateComponent(id, "Ball", b)
            end
        end
    end
end

ECS.registerSystem(InputSystem)