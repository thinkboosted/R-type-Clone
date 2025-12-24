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
            
            if mesh.texturePath then
                print("[RenderSystem] Sending SetTexture for " .. id .. " (Model: " .. tostring(mesh.modelPath) .. ") with path: " .. mesh.texturePath) -- debug
                ECS.sendMessage("RenderEntityCommand", "SetTexture:" .. id .. ":" .. mesh.texturePath)
            else
                if mesh.modelPath ~= "assets/models/cube.obj" then
                    print("[RenderSystem] No texture path for " .. id .. " (Model: " .. tostring(mesh.modelPath) .. ")")
                    -- Debug: print keys
                    for k, v in pairs(mesh) do
                        print("  Key: " .. tostring(k) .. ", Value: " .. tostring(v))
                    end
                end
            end

            RenderSystem.initializedEntities[id] = true
        end

        if color then
            ECS.sendMessage("RenderEntityCommand", "SetColor:" .. id .. "," .. color.r .. "," .. color.g .. "," .. color.b)
        end

        ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. transform.x .. "," .. transform.y .. "," .. transform.z)
        ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. transform.rx .. "," .. transform.ry .. "," .. transform.rz)
    end

    -- Handle Text Entities
    local textEntities = ECS.getEntitiesWith({"Transform", "Text"})
    for _, id in ipairs(textEntities) do
        local transform = ECS.getComponent(id, "Transform")
        local text = ECS.getComponent(id, "Text")
        local color = ECS.getComponent(id, "Color")

        if not RenderSystem.initializedEntities[id] then
            ECS.createText(id, text.text, text.fontPath, text.fontSize, text.isScreenSpace)
            ECS.sendMessage("RenderEntityCommand", "SetScale:" .. id .. "," .. transform.sx .. "," .. transform.sy .. "," .. transform.sz)
            RenderSystem.initializedEntities[id] = true
        end

        if color then
            ECS.sendMessage("RenderEntityCommand", "SetColor:" .. id .. "," .. color.r .. "," .. color.g .. "," .. color.b)
        end

        ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. transform.x .. "," .. transform.y .. "," .. transform.z)
        ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. transform.rx .. "," .. transform.ry .. "," .. transform.rz)

        if not RenderSystem.lastText then RenderSystem.lastText = {} end
        if RenderSystem.lastText[id] ~= text.text then
             ECS.setText(id, text.text)
             RenderSystem.lastText[id] = text.text
        end
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
