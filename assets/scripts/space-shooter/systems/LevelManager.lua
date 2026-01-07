local LevelManager = {}

function LevelManager.init()
    print("[LevelManager] Initialized")
    ECS.subscribe("GAME_STARTED", LevelManager.onGameStarted)
    ECS.subscribe("GAME_RESET", LevelManager.onGameReset)
end

function LevelManager.onGameStarted(msg)
    if not ECS.capabilities.hasAuthority then return end

    print("[LevelManager] Game Started - Loading Level...")

    -- Determine which level to load
    -- For now, simplistic logic: Always Level 1, or read from a config/variable
    -- Ideally, GameState could hold "currentLevelIndex"
    
    local levelIndex = 1
    -- Try to read from current_level.txt if in local/solo mode, or default to 1
    if ECS.capabilities.isLocalMode then
        local file = io.open("current_level.txt", "r")
        if file then
            local lvl = file:read("*n")
            file:close()
            if lvl then levelIndex = lvl end
        end
    end

    local path = "assets/scripts/space-shooter/levels/Level-" .. levelIndex .. ".lua"
    
    -- Check if file exists (basic check by trying to open)
    local f = io.open(path, "r")
    if f then
        f:close()
        print("[LevelManager] Loading level file: " .. path)
        dofile(path)
    else
        print("[LevelManager] ERROR: Level file not found: " .. path .. ". Loading Level 1 fallback.")
        dofile("assets/scripts/space-shooter/levels/Level-1.lua")
    end
end

function LevelManager.onGameReset(msg)
    if not ECS.capabilities.hasAuthority then return end
    print("[LevelManager] Game Reset - Cleaning up level entities is handled by NetworkSystem/LifeSystem")
    -- If LevelManager needs to reset internal state (like wave counter), do it here.
end

ECS.registerSystem(LevelManager)
return LevelManager