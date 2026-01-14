-- AnimationSystem.lua
-- Handles frame-by-frame animation by swapping Model and Texture paths

local AnimationSystem = {}

function AnimationSystem.init()
    print("[AnimationSystem] Initialized")
end

function AnimationSystem.update(dt)
    -- Iterate over entities with Animation and Mesh components
    local entities = ECS.getEntitiesWith({"Animation", "Mesh"})
    
    for _, entityId in ipairs(entities) do
        local anim = ECS.getComponent(entityId, "Animation")
        -- print("Entity " .. entityId .. " anim state: " .. tostring(anim.elapsedTime) .. "/" .. tostring(anim.frameDuration))
        local changed = false

        -- Default inactive/looping states if nil
        if anim.isActive == nil then anim.isActive = true end
        if anim.isLooping == nil then anim.isLooping = true end

        -- print("Entity " .. entityId .. " active: " .. tostring(anim.isActive) .. " elapsed: " .. anim.elapsedTime)

        if anim.isActive then

            anim.elapsedTime = anim.elapsedTime + dt
            
            if anim.elapsedTime >= anim.frameDuration then
                -- Move to next frame
                while anim.elapsedTime >= anim.frameDuration do
                    anim.elapsedTime = anim.elapsedTime - anim.frameDuration
                    anim.currentFrame = anim.currentFrame + 1
                    
                    -- Handle Loop / End
                    if anim.currentFrame > anim.frameCount then
                        if anim.isLooping then
                            anim.currentFrame = 1
                        else
                            anim.currentFrame = anim.frameCount
                            anim.isActive = false
                        end
                    end
                    changed = true
                end
                
                -- Update Mesh Component
                if changed then
                    -- We assume standard patterns: modelBase + X + .obj | textureBase + X + .png
                    if anim.modelBase and anim.modelBase ~= "" then
                        local newModel = anim.modelBase .. math.floor(anim.currentFrame) .. ".obj"
                        local newTexture = ""
                        
                        if anim.textureBase and anim.textureBase ~= "" then
                            newTexture = anim.textureBase .. math.floor(anim.currentFrame) .. ".png"
                        end
                        
                        -- Update the component
                        local mesh = ECS.getComponent(entityId, "Mesh")
                        
                        mesh.modelPath = newModel
                        if newTexture ~= "" then
                            mesh.texturePath = newTexture
                        end
                        
                        -- Force update
                        ECS.addComponent(entityId, "Mesh", mesh)
                    end
                end
            end
            
            -- ALWAYS Persist Animation state changes (elapsedTime updates)
            ECS.addComponent(entityId, "Animation", anim)
        end
    end
end

ECS.registerSystem(AnimationSystem)
