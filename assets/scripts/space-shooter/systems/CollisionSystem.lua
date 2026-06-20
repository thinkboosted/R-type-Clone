-- ============================================================================
-- CollisionSystem.lua - Server-Authoritative Collision Resolution
-- ============================================================================

local config = dofile("assets/scripts/space-shooter/config.lua")
local CollisionSystem = {}

-- Mémoire persistante pour ne pas recréer les corps à l'infini
CollisionSystem.initializedEntities = {}

function CollisionSystem.init()
    print("[CollisionSystem] Initialized")
    -- Note: L'enregistrement se fait à la fin du fichier
end

function CollisionSystem.update(dt)
    if ECS.isPaused then return end

    -- ⚠️ AUTHORITY CHECK: Seul le serveur (ou mode solo) gère la physique
    if not ECS.capabilities.hasAuthority then return end
    if not ECS.isGameRunning then return end

    local entities = ECS.getEntitiesWith({"Transform", "Collider"})

    for _, id in ipairs(entities) do
        -- Si l'entité n'a pas encore de corps physique connu
        if not CollisionSystem.initializedEntities[id] then
            
            local transform = ECS.getComponent(id, "Transform")
            local collider = ECS.getComponent(id, "Collider")
            local physic = ECS.getComponent(id, "Physic")

            -- 1. Préparation des paramètres
            local params = ""
            local colliderTypeUpper = string.upper(collider.type) -- Force MAJUSCULE

            if colliderTypeUpper == "BOX" then
                local sx, sy, sz
                if type(collider.size) == "table" then
                    sx = collider.size[1] or 1
                    sy = collider.size[2] or sx
                    sz = collider.size[3] or sx
                elseif type(collider.size) == "number" then
                    sx, sy, sz = collider.size, collider.size, collider.size
                else
                    sx, sy, sz = 1, 1, 1
                end
                params = sx .. "," .. sy .. "," .. sz

            elseif colliderTypeUpper == "SPHERE" then
                local r = (type(collider.size) == "table") and collider.size[1] or ((type(collider.size) == "number") and collider.size or 1)
                params = tostring(r)
            end

            -- 2. Gestion Physique (Masse & Friction)
            local mass = 0.0
            local friction = 0.5
            local fixedRotation = false
            if physic then
                mass = physic.mass
                friction = physic.friction
                fixedRotation = physic.fixedRotation
            end

            -- 3. Envoi de la commande de création (Avec le Type en MAJUSCULE)
            -- IMPORTANT : On utilise colliderTypeUpper ici !
            local msg = "CreateBody:" .. id .. ":" .. colliderTypeUpper .. ":" .. params .. "," .. mass .. "," .. friction .. ";"
            ECS.sendMessage("PhysicCommand", msg)
            
            print("[CollisionSystem] Created Body for " .. id .. " (" .. colliderTypeUpper .. ")")

            -- 4. Options supplémentaires
            if fixedRotation then
                ECS.sendMessage("PhysicCommand", "SetAngularFactor:" .. id .. ":0,0,0;")
            end

            -- 5. Synchro Initiale : On place le corps physique exactement là où est le visuel
            local tMsg = "SetTransform:" .. id .. ":" .. transform.x .. "," .. transform.y .. "," .. transform.z .. ":" .. transform.rx .. "," .. transform.ry .. "," .. transform.rz .. ";"
            ECS.sendMessage("PhysicCommand", tMsg)

            -- 6. Verrouillage : On note qu'on a traité cette entité
            CollisionSystem.initializedEntities[id] = true
        end
    end
end

function CollisionSystem.hasTag(id, tag)
    local tagComp = ECS.getComponent(id, "Tag")
    if tagComp and tagComp.tags then
        for _, t in ipairs(tagComp.tags) do
            if t == tag then
                return true
            end
        end
    end
    return false
end

function CollisionSystem.onCollision(id1, id2)
    if (CollisionSystem.hasTag(id1, "Bullet") and (CollisionSystem.hasTag(id2, "Bonus") or CollisionSystem.hasTag(id2, "PowerUp"))) or
       (CollisionSystem.hasTag(id2, "Bullet") and (CollisionSystem.hasTag(id1, "Bonus") or CollisionSystem.hasTag(id1, "PowerUp"))) then
        return
    end

    -- Check Player vs Enemy
    if CollisionSystem.hasTag(id1, "Player") and CollisionSystem.hasTag(id2, "Enemy") then
        CollisionSystem.handlePlayerEnemy(id1, id2)
    elseif CollisionSystem.hasTag(id2, "Player") and CollisionSystem.hasTag(id1, "Enemy") then
        CollisionSystem.handlePlayerEnemy(id2, id1)
    end

    -- Check Enemy vs Bullet
    if CollisionSystem.hasTag(id1, "Enemy") and CollisionSystem.hasTag(id2, "Bullet") then
        CollisionSystem.handleEnemyBullet(id1, id2)
    elseif CollisionSystem.hasTag(id2, "Enemy") and CollisionSystem.hasTag(id1, "Bullet") then
        CollisionSystem.handleEnemyBullet(id2, id1)
    end

    -- Check Player vs Enemy attack
    if CollisionSystem.hasTag(id1, "Player") and (CollisionSystem.hasTag(id2, "Enemy") or CollisionSystem.hasTag(id2, "EnemyBullet")) then
            CollisionSystem.handlePlayerEnemy(id1, id2)
    elseif CollisionSystem.hasTag(id2, "Player") and (CollisionSystem.hasTag(id1, "Enemy") or CollisionSystem.hasTag(id1, "EnemyBullet")) then
        CollisionSystem.handlePlayerEnemy(id2, id1)
    end

    -- Check Player vs Bonus
    if CollisionSystem.hasTag(id1, "Player") and CollisionSystem.hasTag(id2, "Bonus") then
        CollisionSystem.handlePlayerBonus(id1, id2)
elseif CollisionSystem.hasTag(id2, "Player") and CollisionSystem.hasTag(id1, "Bonus") then
        CollisionSystem.handlePlayerBonus(id2, id1)
    end

    -- Check Player vs PowerUp (new explicit tag)
    if CollisionSystem.hasTag(id1, "Player") and CollisionSystem.hasTag(id2, "PowerUp") then
        CollisionSystem.handlePlayerBonus(id1, id2)
    elseif CollisionSystem.hasTag(id2, "Player") and CollisionSystem.hasTag(id1, "PowerUp") then
        CollisionSystem.handlePlayerBonus(id2, id1)
    end
end

function CollisionSystem.handlePlayerEnemy(playerId, enemyId)
    -- ⚠️ AUTHORITY: Only server/solo modifies life (guaranteed by update() check)
    local life = ECS.getComponent(playerId, "Life")
    local color = ECS.getComponent(playerId, "Color")

    if CollisionSystem.hasTag(enemyId, "EnemyBullet") then
        local bLife = ECS.getComponent(enemyId, "Life")
        if bLife then bLife.amount = 0 end
    end

    if life then
        if life.invulnerableTime and life.invulnerableTime > 0 then
            return
        end
        
        local damage = CollisionSystem.hasTag(enemyId, "EnemyBullet") and 10 or 25  -- Bullets do less damage
        -- Set invulnerability to prevent rapid repeated damage
        local invulnTime = CollisionSystem.hasTag(enemyId, "EnemyBullet") and 0.3 or 0.6  -- Shorter invuln for bullets
        
        life.amount = life.amount - damage
        life.invulnerableTime = invulnTime
        
        -- Play hit sound
        if not ECS.capabilities.hasNetworkSync then
            ECS.sendMessage("SoundPlay", "player_hit:effects/hit.wav:" .. ECS.getSfxVolume(100))
        else
            -- Broadcast hit sound to all clients
            ECS.broadcastNetworkMessage("PLAY_SOUND", "player_hit:effects/hit.wav:100")
        end
            if color then
                color.r = 1.0
                color.g = 0.0
                color.b = 0.0
            ECS.addComponent(playerId, "Color", color)
        end
        -- Broadcast hit event for multiplayer visuals
        ECS.broadcastNetworkMessage("ENTITY_HIT", playerId)
    end
end

function CollisionSystem.handleEnemyBullet(enemyId, bulletId)
    -- ⚠️ AUTHORITY: Only server/solo modifies life and score
    local life = ECS.getComponent(enemyId, "Life")
    if life then
        local bulletComp = ECS.getComponent(bulletId, "Bullet")
        local damage = (bulletComp and bulletComp.damage) or 1
        if CollisionSystem.hasTag(enemyId, "Boss") then
            life.amount = life.amount - damage
        else
            -- Keep fast clear for normal enemies.
            life.amount = 0
        end
        
        -- Play explosion sound
        if not ECS.capabilities.hasNetworkSync then
            ECS.sendMessage("SoundPlay", "explosion_" .. enemyId .. ":effects/explosion.wav:" .. ECS.getSfxVolume(90))
        else
            -- Broadcast explosion sound to all clients
            ECS.broadcastNetworkMessage("PLAY_SOUND", "explosion_" .. enemyId .. ":effects/explosion.wav:90")
        end
    end

    -- Increase score for the player who shot the bullet
    local ownerComp = ECS.getComponent(bulletId, "Owner")
    if ownerComp and ownerComp.id then
        local playerScore = ECS.getComponent(ownerComp.id, "Score")
        if playerScore then
            playerScore.value = playerScore.value + config.score.kill
        end
    end

    -- Increase global score
    local scoreEntities = ECS.getEntitiesWith({"Score"})
    if #scoreEntities > 0 then
        local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
        scoreComp.value = scoreComp.value + config.score.kill
    end

    -- Destroy bullet after hit
    local bLife = ECS.getComponent(bulletId, "Life")
    if bLife then
        bLife.amount = 0
    end
end

function CollisionSystem.handlePlayerBonus(playerId, bonusId)
    -- ⚠️ AUTHORITY: Only server/solo can grant powerups and destroy bonus
    local bonus = ECS.getComponent(bonusId, "Bonus")
    if bonus then
        local duration = bonus.duration or 8.0
        local pType = bonus.type or "RAPID"

        if PowerUpSystem and PowerUpSystem.applyPowerUp then
            PowerUpSystem.applyPowerUp(playerId, pType, duration)
        else
            -- Safety path: still apply a real weapon buff if PowerUpSystem isn't in scope.
            local weapon = ECS.getComponent(playerId, "Weapon")
            local profile = ECS.getComponent(playerId, "WeaponProfile")
            if weapon then
                if not profile then
                    profile = WeaponProfile("STANDARD", weapon.cooldown or 0.2)
                end

                if pType == "RAPID" then
                    weapon.cooldown = 0.1
                    profile.weaponType = "STANDARD"
                elseif pType == "SPREAD" then
                    weapon.cooldown = 0.22
                    profile.weaponType = "SPREAD"
                elseif pType == "BURST" then
                    weapon.cooldown = 0.28
                    profile.weaponType = "BURST"
                end

                ECS.addComponent(playerId, "Weapon", weapon)
                ECS.addComponent(playerId, "WeaponProfile", profile)
            end
            ECS.addComponent(playerId, "PowerUp", { timeRemaining = duration, originalCooldown = (profile and profile.baseCooldown) or 0.2, powerType = pType })
        end
        
        -- Play powerup sound
        if not ECS.capabilities.hasNetworkSync then
            ECS.sendMessage("SoundPlay", "powerup:effects/powerup.wav:" .. ECS.getSfxVolume(100))
        else
            -- Broadcast powerup sound to all clients
            ECS.broadcastNetworkMessage("PLAY_SOUND", "powerup:effects/powerup.wav:100")
        end

        -- Destroy bonus after collection
        local bonusLife = ECS.getComponent(bonusId, "Life")
        if bonusLife then
            bonusLife.amount = 0
        end
    end
end

ECS.registerSystem(CollisionSystem)

return CollisionSystem
