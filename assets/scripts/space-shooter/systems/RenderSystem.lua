-- ============================================================================
-- RenderSystem.lua - Rendering Only on Capable Instances
-- ============================================================================
-- Only runs on instances with hasRendering capability (Client or Solo).
-- Server instances skip all rendering to save resources.
-- ============================================================================
local RenderSystem = {}
local activeCameraId = nil  -- Track the currently active camera
RenderSystem.initializedEntities = {}

local function nearlyEqual(a, b, eps)
    eps = eps or 0.0001
    return math.abs((a or 0) - (b or 0)) <= eps
end

local function transformChanged(cache, t)
    return not (nearlyEqual(cache.x, t.x) and nearlyEqual(cache.y, t.y) and nearlyEqual(cache.z, t.z)
    and nearlyEqual(cache.rx, t.rx) and nearlyEqual(cache.ry, t.ry) and nearlyEqual(cache.rz, t.rz)
    and nearlyEqual(cache.sx, t.sx) and nearlyEqual(cache.sy, t.sy) and nearlyEqual(cache.sz, t.sz))
end

local function colorChanged(cache, c)
    return not (nearlyEqual(cache.cr, c.r) and nearlyEqual(cache.cg, c.g) and nearlyEqual(cache.cb, c.b))
end

function RenderSystem.init()
    print("[RenderSystem] Initialized (hasRendering: " .. tostring(ECS.capabilities.hasRendering) .. ")")
end

function RenderSystem.update(dt)
    -- Only render on instances with rendering capability
    if not ECS.capabilities.hasRendering then
        return
    end
    
    -- Handle Camera - find and activate the first active camera
    local cameras = ECS.getEntitiesWith({"Transform", "Camera"})
    local foundActiveCamera = false
    
    for _, id in ipairs(cameras) do
        local cam = ECS.getComponent(id, "Camera")
        if cam and cam.isActive then
            foundActiveCamera = true
            -- If this is a new camera or different from current, activate it
            if activeCameraId ~= id then
                print("[RenderSystem] Activating Camera " .. id)
                ECS.sendMessage("RenderEntityCommand", "SetActiveCamera:" .. id)
                activeCameraId = id
            end
            
            local t = ECS.getComponent(id, "Transform")
            ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. t.x .. "," .. t.y .. "," .. t.z)
            ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. t.rx .. "," .. t.ry .. "," .. t.rz)
            break
        end
    end
    
    -- If active camera was destroyed, reset tracking
    if not foundActiveCamera then
        activeCameraId = nil
    end

    -- Handle Mesh Entities
    local entities = ECS.getEntitiesWith({"Transform", "Mesh"})
    for _, id in ipairs(entities) do
        local transform = ECS.getComponent(id, "Transform")
        local mesh = ECS.getComponent(id, "Mesh")
        local color = ECS.getComponent(id, "Color")

        -- Initialize state tracking if needed
        if not RenderSystem.initializedEntities[id] then
            RenderSystem.initializedEntities[id] = {
                model = nil,
                texture = nil,
                x = nil, y = nil, z = nil,
                rx = nil, ry = nil, rz = nil,
                sx = nil, sy = nil, sz = nil,
                cr = nil, cg = nil, cb = nil,
            }
        end
        local cached = RenderSystem.initializedEntities[id]

        -- Check for Model Change (Re-create entity if model changes)
        if cached.model ~= mesh.modelPath then
             local type = mesh.modelPath
             ECS.sendMessage("RenderEntityCommand", "CreateEntity:" .. type .. ":" .. id)
             ECS.sendMessage("RenderEntityCommand", "SetScale:" .. id .. "," .. transform.sx .. "," .. transform.sy .. "," .. transform.sz)
             cached.model = mesh.modelPath
               cached.sx, cached.sy, cached.sz = transform.sx, transform.sy, transform.sz
             
             -- Force texture update after model change
             cached.texture = nil 
        end

        -- Check for Texture Change
        if cached.texture ~= mesh.texturePath then
            if mesh.texturePath then
                ECS.sendMessage("RenderEntityCommand", "SetTexture:" .. id .. ":" .. mesh.texturePath)
            end
            cached.texture = mesh.texturePath
        end

        if color and colorChanged(cached, color) then
            ECS.sendMessage("RenderEntityCommand", "SetColor:" .. id .. "," .. color.r .. "," .. color.g .. "," .. color.b)
            cached.cr, cached.cg, cached.cb = color.r, color.g, color.b
        end

        if transformChanged(cached, transform) then
            if not (nearlyEqual(cached.sx, transform.sx) and nearlyEqual(cached.sy, transform.sy) and nearlyEqual(cached.sz, transform.sz)) then
                ECS.sendMessage("RenderEntityCommand", "SetScale:" .. id .. "," .. transform.sx .. "," .. transform.sy .. "," .. transform.sz)
            end
            ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. transform.x .. "," .. transform.y .. "," .. transform.z)
            ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. transform.rx .. "," .. transform.ry .. "," .. transform.rz)
            cached.x, cached.y, cached.z = transform.x, transform.y, transform.z
            cached.rx, cached.ry, cached.rz = transform.rx, transform.ry, transform.rz
            cached.sx, cached.sy, cached.sz = transform.sx, transform.sy, transform.sz
        end
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
            RenderSystem.initializedEntities[id] = {
                x = nil, y = nil, z = nil,
                rx = nil, ry = nil, rz = nil,
                sx = nil, sy = nil, sz = nil,
                cr = nil, cg = nil, cb = nil,
            }
        end
        local cached = RenderSystem.initializedEntities[id]

        if color and colorChanged(cached, color) then
            ECS.sendMessage("RenderEntityCommand", "SetColor:" .. id .. "," .. color.r .. "," .. color.g .. "," .. color.b)
            cached.cr, cached.cg, cached.cb = color.r, color.g, color.b
        end

        if transformChanged(cached, transform) then
            if not (nearlyEqual(cached.sx, transform.sx) and nearlyEqual(cached.sy, transform.sy) and nearlyEqual(cached.sz, transform.sz)) then
                ECS.sendMessage("RenderEntityCommand", "SetScale:" .. id .. "," .. transform.sx .. "," .. transform.sy .. "," .. transform.sz)
            end
            ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. transform.x .. "," .. transform.y .. "," .. transform.z)
            ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. transform.rx .. "," .. transform.ry .. "," .. transform.rz)
            cached.x, cached.y, cached.z = transform.x, transform.y, transform.z
            cached.rx, cached.ry, cached.rz = transform.rx, transform.ry, transform.rz
            cached.sx, cached.sy, cached.sz = transform.sx, transform.sy, transform.sz
        end

        if not RenderSystem.lastText then RenderSystem.lastText = {} end
        if RenderSystem.lastText[id] ~= text.text then
             ECS.setText(id, text.text)
             RenderSystem.lastText[id] = text.text
        end
    end
end

ECS.registerSystem(RenderSystem)
