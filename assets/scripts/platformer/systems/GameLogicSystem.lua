-- Game Logic System for 3D Platformer
-- Handles checkpoints, death floor, and finish line

GameLogicSystem = {}

-- Store the current checkpoint position for respawning
GameLogicSystem.lastCheckpoint = {x = 0, y = 2, z = 0}
GameLogicSystem.currentLevel = 1
GameLogicSystem.gameWon = false
GameLogicSystem.winMessageTimer = 0

function GameLogicSystem.init()
    print("[GameLogicSystem] Initialized")
end

function GameLogicSystem.update(dt)
    -- Update win message timer
    if GameLogicSystem.gameWon then
        GameLogicSystem.winMessageTimer = GameLogicSystem.winMessageTimer + dt
    end

    -- Check if player fell too far (death floor check)
    local players = ECS.getEntitiesWith({"Player", "Transform"})
    if #players > 0 then
        local playerId = players[1]
        local transform = ECS.getComponent(playerId, "Transform")
        
        -- If player falls below -10, respawn at checkpoint
        if transform.y < -10 then
            GameLogicSystem.respawnPlayer(playerId)
        end
    end
end

function GameLogicSystem.respawnPlayer(playerId)
    local transform = ECS.getComponent(playerId, "Transform")
    local physic = ECS.getComponent(playerId, "Physic")
    
    print("[GameLogicSystem] Respawning player at checkpoint")
    
    -- Reset player position to last checkpoint
    transform.x = GameLogicSystem.lastCheckpoint.x
    transform.y = GameLogicSystem.lastCheckpoint.y
    transform.z = GameLogicSystem.lastCheckpoint.z
    
    -- Reset velocities
    if physic then
        physic.vx = 0
        physic.vy = 0
        physic.vz = 0
    end
    
    -- Update physics engine position
    local tMsg = "SetTransform:" .. playerId .. ":" .. transform.x .. "," .. transform.y .. "," .. transform.z .. ":" .. transform.rx .. "," .. transform.ry .. "," .. transform.rz .. ";"
    ECS.sendMessage("PhysicCommand", tMsg)
    
    -- Reset velocity in physics engine
    ECS.sendMessage("PhysicCommand", "SetVelocity:" .. playerId .. ":0,0,0;")
end

function GameLogicSystem.onCollision(id1, id2)
    local player1 = ECS.getComponent(id1, "Player")
    local player2 = ECS.getComponent(id2, "Player")
    
    local checkpoint1 = ECS.getComponent(id1, "Checkpoint")
    local checkpoint2 = ECS.getComponent(id2, "Checkpoint")
    
    local finishLine1 = ECS.getComponent(id1, "FinishLine")
    local finishLine2 = ECS.getComponent(id2, "FinishLine")
    
    local deathFloor1 = ECS.getComponent(id1, "DeathFloor")
    local deathFloor2 = ECS.getComponent(id2, "DeathFloor")
    
    -- Check checkpoint collision
    if (player1 and checkpoint2) then
        GameLogicSystem.activateCheckpoint(id2)
    elseif (player2 and checkpoint1) then
        GameLogicSystem.activateCheckpoint(id1)
    end
    
    -- Check finish line collision
    if (player1 and finishLine2) then
        GameLogicSystem.finishLevel(finishLine2)
    elseif (player2 and finishLine1) then
        GameLogicSystem.finishLevel(finishLine1)
    end
    
    -- Check death floor collision
    if (player1 and deathFloor2) then
        GameLogicSystem.respawnPlayer(id1)
    elseif (player2 and deathFloor1) then
        GameLogicSystem.respawnPlayer(id2)
    end
end

function GameLogicSystem.activateCheckpoint(checkpointId)
    local checkpoint = ECS.getComponent(checkpointId, "Checkpoint")
    local transform = ECS.getComponent(checkpointId, "Transform")
    
    if not checkpoint.activated then
        checkpoint.activated = true
        GameLogicSystem.lastCheckpoint.x = transform.x
        GameLogicSystem.lastCheckpoint.y = transform.y + 2
        GameLogicSystem.lastCheckpoint.z = transform.z
        
        print("[GameLogicSystem] Checkpoint " .. checkpoint.id .. " activated!")
        
        -- Play checkpoint sound
        ECS.sendMessage("SoundPlay", "checkpoint:effects/powerup.wav:80")
        
        -- Change checkpoint color to green
        local color = ECS.getComponent(checkpointId, "Color")
        if color then
            color.r = 0
            color.g = 1
            color.b = 0
        end
    end
end

function GameLogicSystem.finishLevel(finishLine)
    print("[GameLogicSystem] Level complete!")
    
    if finishLine.nextLevelScript ~= "" then
        GameLogicSystem.currentLevel = GameLogicSystem.currentLevel + 1
        GameLogicSystem.gameWon = false
        ECS.sendMessage("LevelComplete", finishLine.nextLevelScript)
    else
        print("[GameLogicSystem] *** CONGRATULATIONS! YOU BEAT THE GAME! ***")
        GameLogicSystem.gameWon = true
        GameLogicSystem.winMessageTimer = 0
    end
end

ECS.registerSystem(GameLogicSystem)
