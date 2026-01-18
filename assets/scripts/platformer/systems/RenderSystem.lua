-- Render System for 3D Platformer

RenderSystem = {}
RenderSystem.CameraInitialized = false
RenderSystem.initializedEntities = {}

function RenderSystem.init()
    print("[RenderSystem] Initialized")
end

function RenderSystem.update(dt)
    -- Initialize camera
    local cameras = ECS.getEntitiesWith({"Transform", "Camera"})
    for _, id in ipairs(cameras) do
        local cam = ECS.getComponent(id, "Camera")
        if cam.isActive and not RenderSystem.CameraInitialized then
            ECS.sendMessage("RenderEntityCommand", "SetActiveCamera:" .. id)
            RenderSystem.CameraInitialized = true

            local t = ECS.getComponent(id, "Transform")
            ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. t.x .. "," .. t.y .. "," .. t.z)
            ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. t.rx .. "," .. t.ry .. "," .. t.rz)
            break
        end
    end

    -- Initialize and update lights (position relative to camera for better visibility)
    local lights = ECS.getEntitiesWith({"Transform", "Light"})
    for _, id in ipairs(lights) do
        local light = ECS.getComponent(id, "Light")

        if not RenderSystem.initializedEntities[id] then
            ECS.sendMessage("RenderEntityCommand", "CreateEntity:Light:" .. id)
            ECS.sendMessage("RenderEntityCommand", "SetLightProperties:" .. id .. "," .. light.r .. "," .. light.g .. "," .. light.b .. "," .. light.intensity)
            RenderSystem.initializedEntities[id] = true
        end

        -- Make light follow camera position for consistent lighting
        if #cameras > 0 then
            local cameraId = cameras[1]
            local cameraTransform = ECS.getComponent(cameraId, "Transform")
            -- Position light slightly above and to the side of camera
            local lightX = cameraTransform.x + 5
            local lightY = cameraTransform.y + 10
            local lightZ = cameraTransform.z + 5
            ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. lightX .. "," .. lightY .. "," .. lightZ)
        end
    end

    -- Render meshes
    local entities = ECS.getEntitiesWith({"Transform", "Mesh"})
    for _, id in ipairs(entities) do
        local transform = ECS.getComponent(id, "Transform")
        local mesh = ECS.getComponent(id, "Mesh")
        local color = ECS.getComponent(id, "Color")

        if not RenderSystem.initializedEntities[id] then
            local type = mesh.modelPath

            ECS.sendMessage("RenderEntityCommand", "CreateEntity:" .. type .. ":" .. id)
            ECS.sendMessage("RenderEntityCommand", "SetScale:" .. id .. "," .. transform.sx .. "," .. transform.sy .. "," .. transform.sz)

            RenderSystem.initializedEntities[id] = true
        end

        if color then
            ECS.sendMessage("RenderEntityCommand", "SetColor:" .. id .. "," .. color.r .. "," .. color.g .. "," .. color.b)
        end

        ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. transform.x .. "," .. transform.y .. "," .. transform.z)
        ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. transform.rx .. "," .. transform.ry .. "," .. transform.rz)
    end

    -- Update camera positions
    for _, id in ipairs(cameras) do
        local transform = ECS.getComponent(id, "Transform")
        ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. transform.x .. "," .. transform.y .. "," .. transform.z)
        ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. transform.rx .. "," .. transform.ry .. "," .. transform.rz)
    end

    -- Render text (UI elements)
    local textEntities = ECS.getEntitiesWith({"Transform", "Text"})
    for _, id in ipairs(textEntities) do
        local text = ECS.getComponent(id, "Text")
        local transform = ECS.getComponent(id, "Transform")

        -- Check if we should display this text (for win message, check game state)
        local shouldDisplay = false
        if GameLogicSystem and GameLogicSystem.gameWon then
            shouldDisplay = true
        end

        if shouldDisplay then
            if not RenderSystem.initializedEntities[id] then
                -- Create text entity
                local isScreenSpaceNum = text.isScreenSpace and 1 or 0
                ECS.sendMessage("RenderEntityCommand", "CreateText:" .. id .. ":" .. 
                    text.fontPath .. ":" .. text.fontSize .. ":" .. isScreenSpaceNum .. ":" .. text.content)
                
                -- Set color
                ECS.sendMessage("RenderEntityCommand", "SetColor:" .. id .. "," .. text.r .. "," .. text.g .. "," .. text.b)
                
                RenderSystem.initializedEntities[id] = true
            end
            
            -- Update position (center of screen for win message)
            ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. transform.x .. "," .. transform.y .. "," .. transform.z)
            
            -- Animate with pulse effect
            if GameLogicSystem and GameLogicSystem.winMessageTimer then
                local pulse = 0.9 + 0.1 * math.sin(GameLogicSystem.winMessageTimer * 3)
                ECS.sendMessage("RenderEntityCommand", "SetScale:" .. id .. "," .. pulse .. "," .. pulse .. ",1.0")
            end
        end
    end
end

ECS.registerSystem(RenderSystem)
