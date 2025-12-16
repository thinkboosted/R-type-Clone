local GameStateManager = {}

function GameStateManager.init()
    print("[GameStateManager] Initialized")
    -- Listen for explicit game state transition requests from other systems
    ECS.subscribe("REQUEST_GAME_STATE_CHANGE", GameStateManager.onRequestGameStateChange)
end

function GameStateManager.update(dt)
    -- Only authoritative instance manages the central game state
    if not ECS.capabilities.hasAuthority then return end

    local gsEntities = ECS.getEntitiesWith({"GameState"})
    if #gsEntities == 0 then return end -- No GameState entity to manage

    local gameState = ECS.getComponent(gsEntities[1], "GameState")

    if gameState.transitionRequested then
        print("[GameStateManager] Transitioning game state from " .. gameState.state .. " to " .. gameState.nextState)
        gameState.state = gameState.nextState
        gameState.transitionRequested = false
        gameState.nextState = nil -- Clear the request

        if gameState.state == "PLAYING" then
            print("[GameStateManager] Game State is now PLAYING. Notifying systems.")
            ECS.sendMessage("GAME_STARTED", "") -- Notify other systems (e.g., NetworkSystem)
            ECS.isGameRunning = true -- Ensure global flag is set
        elseif gameState.state == "MENU" then
            print("[GameStateManager] Game State is now MENU. Notifying systems.")
            ECS.sendMessage("GAME_RESET", "") -- Notify other systems
            ECS.isGameRunning = false
        -- Add other state transitions as needed (e.g., GAMEOVER)
        end
    end
end

-- External systems can request a state change
function GameStateManager.onRequestGameStateChange(msg)
    if not ECS.capabilities.hasAuthority then return end

    local gsEntities = ECS.getEntitiesWith({"GameState"})
    if #gsEntities == 0 then return end

    local gameState = ECS.getComponent(gsEntities[1], "GameState")
    local newState = msg

    if newState ~= gameState.state then
        gameState.transitionRequested = true
        gameState.nextState = newState
        print("[GameStateManager] Requested game state change to: " .. newState)
    else
        print("[GameStateManager] Game state already " .. newState .. ". No change.")
    end
end

ECS.registerSystem(GameStateManager)
return GameStateManager
