-- Render System
local RenderSystem = {}
local CameraInitialized = false
RenderSystem.initializedEntities = {}

function RenderSystem.init()
    print("[RenderSystem] Initialized")
end

function RenderSystem.update(dt)
    -- Handle Camera
    local cameras = ECS.getEntitiesWith({"Transform", "Camera"})
    for _, id in ipairs(cameras) do
        local cam = ECS.getComponent(id, "Camera")
        if cam.isActive and not CameraInitialized then
            ECS.sendMessage("RenderEntityCommand", "SetActiveCamera:" .. id)
            CameraInitialized = true

            local t = ECS.getComponent(id, "Transform")
            ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. t.x .. "," .. t.y .. "," .. t.z)
            ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. t.rx .. "," .. t.ry .. "," .. t.rz)
            break
        end
    end

    -- Handle Mesh Entities
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

    -- Update Camera Position (if it moves)
    local cameras = ECS.getEntitiesWith({"Transform", "Camera"})
    for _, id in ipairs(cameras) do
        local transform = ECS.getComponent(id, "Transform")
        ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. transform.x .. "," .. transform.y .. "," .. transform.z)
        ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. transform.rx .. "," .. transform.ry .. "," .. transform.rz)
    end
end

ECS.registerSystem(RenderSystem)
