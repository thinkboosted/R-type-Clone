local PlayerSystem = {}

function PlayerSystem.init()
    print("[PlayerSystem] Initialized")
end

function PlayerSystem.update(dt)
    -- La logique de jeu (tir, cooldown) ne s'exécute que si on a l'autorité
    if not ECS.capabilities.hasAuthority then return end

    local players = ECS.getEntitiesWith({"Player", "InputState", "Transform", "Weapon"})
    
    for _, id in ipairs(players) do
        local input = ECS.getComponent(id, "InputState")
        local weapon = ECS.getComponent(id, "Weapon")
        local transform = ECS.getComponent(id, "Transform")

        weapon.timeSinceLastShot = weapon.timeSinceLastShot + dt

        if input.shoot and weapon.timeSinceLastShot >= weapon.cooldown then
            local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")
            -- Spawn bullet légèrement devant le joueur (+1.5 en X)
            Spawns.spawnBullet(transform.x + 1.5, transform.y, transform.z, false)
            weapon.timeSinceLastShot = 0
            
            -- Broadcast laser sound to all clients
            ECS.broadcastNetworkMessage("PLAY_SOUND", "laser_" .. id .. "_" .. os.time() .. ":effects/laser.wav:80")
            
            print("[PlayerSystem] Player " .. id .. " shot a bullet!")
        end
    end
end

ECS.registerSystem(PlayerSystem)
return PlayerSystem
