local config = dofile("assets/scripts/space-shooter/config.lua")
local Spawns = dofile("assets/scripts/space-shooter/spawns.lua")

local EnemySystem = {}

EnemySystem.spawnTimer = 0
EnemySystem.spawnInterval = config.enemy.spawnInterval or 2.0
EnemySystem.gameTime = 0
EnemySystem.baseSpawnInterval = config.enemy.spawnInterval or 2.0
EnemySystem.baseSpeed = config.enemy.speed or 5.0

local function getGameStateState()
    local entities = ECS.getEntitiesWith({"GameState", "ServerAuthority"})
    if #entities == 0 then
        entities = ECS.getEntitiesWith({"GameState"})
    end
    if #entities == 0 then
        return nil
    end
    local gs = ECS.getComponent(entities[1], "GameState")
    return gs and gs.state or nil
end

function EnemySystem.init()
    print("[EnemySystem] Initialized")
    EnemySystem.resetDifficulty() -- Clear state on init
end

function EnemySystem.update(dt)
    if ECS.isPaused then return end

    -- Only run on authoritative instances (Server or Solo)
    if not ECS.capabilities.hasAuthority then return end
    if not ECS.isGameRunning then return end
    local gsState = getGameStateState()
    if gsState and gsState ~= "PLAYING" then return end

    -- Wait for players to join before simulating enemies
    local players = ECS.getEntitiesWith({"Player"})
    -- print("DEBUG: EnemySystem found " .. #players .. " players") -- Uncomment for spam debug
    if #players == 0 then
        return
    end

    EnemySystem.spawnTimer = EnemySystem.spawnTimer + dt
    if EnemySystem.spawnTimer >= EnemySystem.spawnInterval then
        EnemySystem.spawnTimer = 0
        EnemySystem.spawnEnemy()
    end

    -- Update Enemies (Movement & Bounds)
    local enemies = ECS.getEntitiesWith({"Enemy", "Transform"})
    for _, id in ipairs(enemies) do
        local t = ECS.getComponent(id, "Transform")
        
        -- Handle Movement Patterns
        local movement = ECS.getComponent(id, "MovementPattern")
        if movement then
            movement.time = movement.time + dt
            
            if movement.patternType == "sine" then
                -- Apply Sine Wave logic to vertical velocity
                -- dy/dt of A*sin(w*t) is A*w*cos(w*t)
                local phys = ECS.getComponent(id, "Physic")
                if phys then
                    -- V_y = Amplitude * Frequency * cos(Frequency * Time)
                    phys.vy = movement.amplitude * movement.frequency * math.cos(movement.frequency * movement.time)
                    ECS.addComponent(id, "Physic", phys)
                end

            elseif movement.patternType == "zigzag" then
                -- ZigZag: Constant vertical speed, flipping direction
                local phys = ECS.getComponent(id, "Physic")
                if phys then
                    -- Triangle wave derivative is a square wave
                    -- Frequency f -> Period T = 1/f
                    -- Slope = 4 * Amplitude * Frequency
                    local period = 1.0 / movement.frequency
                    local phase = (movement.time % period) / period -- 0.0 to 1.0
                    local slope = 4 * movement.amplitude * movement.frequency
                    
                    if phase < 0.5 then
                        phys.vy = slope
                    else
                        phys.vy = -slope
                    end
                    ECS.addComponent(id, "Physic", phys)
                end

            elseif movement.patternType == "circle" then
                -- Circular motion (Loop-the-loop)
                local phys = ECS.getComponent(id, "Physic")
                if phys then
                    local w = movement.frequency
                    local R = movement.amplitude
                    
                    -- vx = -speed + (-R * w * sin(w*t))
                    -- vy = R * w * cos(w*t)
                    phys.vx = -movement.speed + (R * w * -math.sin(w * movement.time))
                    phys.vy = R * w * math.cos(w * movement.time)
                    
                    ECS.addComponent(id, "Physic", phys)
                end
            
            elseif movement.patternType == "figure8" then
                -- Figure-8 (Lissajous: Y=sin(t), X=sin(2t))
                -- Modulate horizontal speed to Create a '8' shape relative to the moving frame
                local phys = ECS.getComponent(id, "Physic")
                if phys then
                    local w = movement.frequency
                    local R = movement.amplitude
                    
                    -- Y Velocity: Cosine (derivative of sine)
                    phys.vy = R * w * math.cos(w * movement.time)
                    
                    -- X Velocity: -Speed + Derivative of Sin(2t)
                    -- x(t) = B * sin(2wt) -> v(t) = B * 2w * cos(2wt)
                    local B = R * 0.5 -- Width of the '8'
                    phys.vx = -movement.speed + (B * 2 * w * math.cos(2 * w * movement.time))
                    
                    ECS.addComponent(id, "Physic", phys)
                end

            elseif movement.patternType == "step" then
                -- Step / Staircase: Move Left, then Move Down, repeat
                local phys = ECS.getComponent(id, "Physic")
                if phys then
                    local period = 2.0 / movement.frequency
                    local phase = movement.time % period
                    
                    -- 70% time moving Left, 30% time moving Down/Up
                    if phase < period * 0.7 then
                        phys.vx = -movement.speed
                        phys.vy = 0
                    else
                        phys.vx = 0
                        -- Alternate direction every full cycle? Or just erratic steps?
                        -- Using math.sin to determine direction (positive or negative step) based on total time
                        if math.sin(movement.time) > 0 then
                            phys.vy = movement.speed
                        else
                            phys.vy = -movement.speed
                        end
                    end
                    ECS.addComponent(id, "Physic", phys)
                end
            end
            
            ECS.addComponent(id, "MovementPattern", movement)
        end

        -- Check Bounds
        if t.x < -20 then
            -- Decrease score when enemy escapes
            local scoreEntities = ECS.getEntitiesWith({"Score"})
            if #scoreEntities > 0 then
                local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
                scoreComp.value = scoreComp.value - 20
                if scoreComp.value < 0 then
                    scoreComp.value = 0
                end
                ECS.addComponent(scoreEntities[1], "Score", scoreComp)
            end
            -- Broadcast destruction to clients in multiplayer
            if ECS.capabilities.hasNetworkSync then
                ECS.broadcastNetworkMessage("ENTITY_DESTROY", tostring(id))
            end
            ECS.destroyEntity(id)
        end
    end
end

function EnemySystem.spawnEnemy()
    local y = math.random(-5, 5)
    local x = 25

    local difficultyMultiplier = 1.0 + (EnemySystem.gameTime / 30.0)
    local speed = EnemySystem.baseSpeed * difficultyMultiplier
    if config.enemy.maxSpeed and speed > config.enemy.maxSpeed then speed = config.enemy.maxSpeed end
    
    -- Debug Difficulty
    -- print("Spawning Enemy. Time: " .. EnemySystem.gameTime .. " Speed: " .. speed)

    Spawns.spawnEnemy(x, y, speed)
end

-- Hook for NetworkSystem to reset
function EnemySystem.resetDifficulty()
    print("[EnemySystem] Resetting Difficulty")
    EnemySystem.gameTime = 0
    EnemySystem.spawnTimer = 0
    EnemySystem.spawnInterval = EnemySystem.baseSpawnInterval
end

-- Subscribe to global event if possible, or poll?
-- LuaECSManager doesn't allow calling System function from another System easily without event.
-- Let's stick to simple var reset if reloading script, but script stays loaded on server.
-- We can add a subscription to "RESET_DIFFICULTY"
ECS.subscribe("RESET_DIFFICULTY", function(msg)
    EnemySystem.resetDifficulty()
end)

ECS.registerSystem(EnemySystem)
