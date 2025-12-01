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
            -- ID:offsetX,offsetY,offsetZ,dirX,dirY,dirZ,spread,speed,life,rate,size,r,g,b
            local createStr = id .. ":" .. gen.offsetX .. "," .. gen.offsetY .. "," .. gen.offsetZ .. "," ..
                              gen.dirX .. "," .. gen.dirY .. "," .. gen.dirZ .. "," ..
                              gen.spread .. "," .. gen.speed .. "," .. gen.lifeTime .. "," ..
                              gen.rate .. "," .. gen.size .. "," ..
                              gen.r .. "," .. gen.g .. "," .. gen.b

            ECS.sendMessage("RenderEntityCommand", "CreateParticleGenerator:" .. createStr)
            ParticleSystem.initializedParticles[id] = true
        end

        -- Update Transform
        ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. t.x .. "," .. t.y .. "," .. t.z)
        ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. "," .. t.rx .. "," .. t.ry .. "," .. t.rz)
    end
end

ECS.registerSystem(ParticleSystem)
