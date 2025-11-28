local ParticleSystem = {}
ParticleSystem.initializedParticles = {}

function ParticleSystem.init()
    print("[ParticleSystem] Initialized")
end

function ParticleSystem.update(dt)
    local entities = ECS.getEntitiesWith({"Transform", "ParticleGenerator"})
    for _, id in ipairs(entities) do
        local t = ECS.getComponent(id, "Transform")
        local gen = ECS.getComponent(id, "ParticleGenerator")

        if not ParticleSystem.initializedParticles[id] then
            ECS.sendMessage("RenderEntityCommand", "CreateParticleGenerator:" .. id)
            ParticleSystem.initializedParticles[id] = true
        end

        -- Calculate rotated offset
        local ox, oy, oz = gen.offsetX, gen.offsetY, gen.offsetZ

        -- Rotate around Z
        local radZ = math.rad(t.rz)
        local cosZ = math.cos(radZ)
        local sinZ = math.sin(radZ)

        local x1 = ox * cosZ - oy * sinZ
        local y1 = ox * sinZ + oy * cosZ
        local z1 = oz

        -- Rotate around Y
        local radY = math.rad(t.ry)
        local cosY = math.cos(radY)
        local sinY = math.sin(radY)

        local x2 = x1 * cosY + z1 * sinY
        local y2 = y1
        local z2 = -x1 * sinY + z1 * cosY

        -- Rotate around X
        local radX = math.rad(t.rx)
        local cosX = math.cos(radX)
        local sinX = math.sin(radX)

        local x3 = x2
        local y3 = y2 * cosX - z2 * sinX
        local z3 = y2 * sinX + z2 * cosX

        local px = t.x + x3
        local py = t.y + y3
        local pz = t.z + z3

        -- Calculate rotated direction
        local dx, dy, dz = gen.dirX, gen.dirY, gen.dirZ

        -- Rotate direction around Z
        local dx1 = dx * cosZ - dy * sinZ
        local dy1 = dx * sinZ + dy * cosZ
        local dz1 = dz

        -- Rotate direction around Y
        local dx2 = dx1 * cosY + dz1 * sinY
        local dy2 = dy1
        local dz2 = -dx1 * sinY + dz1 * cosY

        -- Rotate direction around X
        local dx3 = dx2
        local dy3 = dy2 * cosX - dz2 * sinX
        local dz3 = dy2 * sinX + dz2 * cosX

        -- ID:x,y,z,dirX,dirY,dirZ,spread,speed,life,rate,size,r,g,b
        local updateStr = id .. ":" .. px .. "," .. py .. "," .. pz .. "," ..
                          dx3 .. "," .. dy3 .. "," .. dz3 .. "," ..
                          gen.spread .. "," .. gen.speed .. "," .. gen.lifeTime .. "," ..
                          gen.rate .. "," .. gen.size .. "," ..
                          gen.r .. "," .. gen.g .. "," .. gen.b

        ECS.sendMessage("RenderEntityCommand", "UpdateParticleGenerator:" .. updateStr)
    end
end

ECS.registerSystem(ParticleSystem)
