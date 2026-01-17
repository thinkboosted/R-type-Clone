local RenderSystem = {}
local initialized = {}

function RenderSystem.init()
    print("[RenderSystem] Init")
end

function RenderSystem.update(dt)
    -- 1. Handle CAMERA (CRITICAL: Must update position/rotation)
    local cameras = ECS.getEntitiesWith({"Transform", "Camera"})
    for _, id in ipairs(cameras) do
        local t = ECS.getComponent(id, "Transform")

        -- Force update transform for Camera
        -- We use manual messages or updateComponent.
        -- updateComponent is safer if bindings exist, but let's be explicit like R-Type's RenderSystem
        ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. t.x .. "," .. t.y .. "," .. t.z)
        ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. t.rx .. "," .. t.ry .. "," .. t.rz)

        -- Ensure it's active (ECS.addComponent("Camera") does this once, but good to be sure)
        -- ECS.sendMessage("RenderEntityCommand", "SetActiveCamera:" .. id)
    end

    -- 2. Handle MESHES
    local entities = ECS.getEntitiesWith({"Transform", "Mesh"})
    for _, id in ipairs(entities) do
        local t = ECS.getComponent(id, "Transform")
        local m = ECS.getComponent(id, "Mesh")

        if not initialized[id] then
            -- Initial Creation
            print("[RenderSystem] Creating Mesh for " .. id .. ": " .. m.modelPath)
            ECS.createMesh(id, m.modelPath)

            if m.texturePath and m.texturePath ~= "" then
                ECS.setTexture(id, m.texturePath)
            end

            -- Set Initial Scale (Default might be 1, but let's sync)
            ECS.sendMessage("RenderEntityCommand", "SetScale:" .. id .. "," .. t.sx .. "," .. t.sy .. "," .. t.sz)

            initialized[id] = true
        end

        -- Sync Transform
        ECS.updateComponent(id, "Transform", t)
    end
end

ECS.registerSystem(RenderSystem)
