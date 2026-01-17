local HitFlashSystem = {}

function HitFlashSystem.update(dt)
    local entities = ECS.getEntitiesWith({"HitFlash", "Color"})
    for _, id in ipairs(entities) do
        local hitFlash = ECS.getComponent(id, "HitFlash")
        hitFlash.timer = hitFlash.timer - dt
        if hitFlash.timer <= 0 then
            local colorComp = ECS.getComponent(id, "Color")
            if colorComp and hitFlash.originalR then
                colorComp.r = hitFlash.originalR
                colorComp.g = hitFlash.originalG
                colorComp.b = hitFlash.originalB
            end
            ECS.removeComponent(id, "HitFlash")
        end
    end
end

ECS.registerSystem(HitFlashSystem)