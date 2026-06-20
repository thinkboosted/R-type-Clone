local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")

local WeaponSystem = {}

local function fireStandard(transform, ownerId)
    Spawns.spawnBullet(transform.x + 1.4, transform.y, transform.z, false, ownerId)
end

local function fireSpread(transform, ownerId)
    Spawns.spawnBullet(transform.x + 1.4, transform.y, transform.z, false, ownerId)
    Spawns.spawnBullet(transform.x + 1.4, transform.y + 0.45, transform.z, false, ownerId)
    Spawns.spawnBullet(transform.x + 1.4, transform.y - 0.45, transform.z, false, ownerId)
end

local function fireBurstFan(transform, ownerId)
    local baseX = transform.x + 1.4
    local baseY = transform.y
    local speed = 20.0

    -- 5-shot fan: center, 2 medium angles, 2 wide angles.
    Spawns.spawnBullet(baseX, baseY, transform.z, false, ownerId, speed, 0)
    Spawns.spawnBullet(baseX, baseY + 0.15, transform.z, false, ownerId, speed * 0.94, 5.0)
    Spawns.spawnBullet(baseX, baseY - 0.15, transform.z, false, ownerId, speed * 0.94, -5.0)
    Spawns.spawnBullet(baseX, baseY + 0.3, transform.z, false, ownerId, speed * 0.88, 9.0)
    Spawns.spawnBullet(baseX, baseY - 0.3, transform.z, false, ownerId, speed * 0.88, -9.0)
end

function WeaponSystem.init()
    print("[WeaponSystem] Initialized")
end

function WeaponSystem.update(dt)
    if ECS.isPaused then return end
    if not ECS.capabilities.hasAuthority then return end

    local players = ECS.getEntitiesWith({"Player", "InputState", "Transform", "Weapon"})
    for _, id in ipairs(players) do
        local input = ECS.getComponent(id, "InputState")
        local weapon = ECS.getComponent(id, "Weapon")
        local profile = ECS.getComponent(id, "WeaponProfile")
        local transform = ECS.getComponent(id, "Transform")

        if not profile then
            profile = WeaponProfile("STANDARD", weapon.cooldown or 0.2)
            ECS.addComponent(id, "WeaponProfile", profile)
        end

        weapon.timeSinceLastShot = (weapon.timeSinceLastShot or 0) + dt

        if input.shoot and weapon.timeSinceLastShot >= weapon.cooldown then
            if profile.weaponType == "SPREAD" then
                fireSpread(transform, id)
            elseif profile.weaponType == "BURST" then
                fireBurstFan(transform, id)
            else
                fireStandard(transform, id)
            end
            weapon.timeSinceLastShot = 0

            if not ECS.capabilities.hasNetworkSync then
                ECS.sendMessage("SoundPlay", "laser_" .. id .. "_" .. os.time() .. ":effects/laser.wav:" .. ECS.getSfxVolume(80))
            else
                ECS.broadcastNetworkMessage("PLAY_SOUND", "laser_" .. id .. "_" .. os.time() .. ":effects/laser.wav:80")
            end
        end
    end
end

ECS.registerSystem(WeaponSystem)
return WeaponSystem
