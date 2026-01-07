local LevelManager = {}

function LevelManager.init()
    print("[LevelManager] Initialized")
    ECS.subscribe("GAME_STARTED", LevelManager.onGameStarted)
end

function LevelManager.onGameStarted(msg)
    if not ECS.capabilities.hasAuthority then return end
    print("[LevelManager] Loading Level 1...")
    dofile("assets/scripts/space-shooter/levels/Level-1.lua")
end

ECS.registerSystem(LevelManager)
return LevelManager