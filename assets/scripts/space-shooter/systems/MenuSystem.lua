local MenuSystem = {}
local Spawns = require("assets/scripts/space-shooter/spawns")

-- ============================================================================
-- STATE GUARD: Empêche le rendu multiple du menu
-- ============================================================================
local isMenuRendered = false
local menuIDs = {}

-- ============================================================================
-- INIT: Appelée une seule fois au démarrage
-- ============================================================================
function MenuSystem.init()
    print("[MenuSystem] Initialized")
    ECS.subscribe("MousePressed", MenuSystem.onMousePressed)

    -- Create GameState entity required by NetworkSystem
    local gs = ECS.createEntity()
    ECS.addComponent(gs, "GameState", GameState("MENU", 0))
    if ECS.capabilities.hasAuthority then
        ECS.addComponent(gs, "ServerAuthority", ServerAuthority())
    end
    print("[MenuSystem] GameState entity created: " .. gs)

    MenuSystem.renderMenu() -- One-shot render
end

-- ============================================================================
-- RENDER MENU: Création des cubes et caméra du menu (ONE SHOT)
-- ============================================================================
function MenuSystem.renderMenu()
    if isMenuRendered then return end -- silent guard

    isMenuRendered = true
    menuIDs = {}

    -- Cube SOLO (Gauche, Vert)
    local solo = ECS.createEntity()
    ECS.addComponent(solo, "Transform", Transform(-8, 0, 0, 0, 0, 0, 2, 2, 2))
    ECS.addComponent(solo, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(solo, "Color", Color(0.0, 1.0, 0.0))
    ECS.addComponent(solo, "Tag", Tag({"MenuUI"}))
    ECS.sendMessage("RenderEntityCommand", "CreateEntity:assets/models/cube.obj:" .. solo)
    ECS.sendMessage("RenderEntityCommand", "SetScale:" .. solo .. ",2,2,2")
    ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. solo .. ",-8,0,0")
    ECS.sendMessage("RenderEntityCommand", "SetColor:" .. solo .. ",0,1,0")
    table.insert(menuIDs, solo)

    -- Cube MULTI (Droite, Bleu)
    local multi = ECS.createEntity()
    ECS.addComponent(multi, "Transform", Transform(8, 0, 0, 0, 0, 0, 2, 2, 2))
    ECS.addComponent(multi, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(multi, "Color", Color(0.0, 0.0, 1.0))
    ECS.addComponent(multi, "Tag", Tag({"MenuUI"}))
    ECS.sendMessage("RenderEntityCommand", "CreateEntity:assets/models/cube.obj:" .. multi)
    ECS.sendMessage("RenderEntityCommand", "SetScale:" .. multi .. ",2,2,2")
    ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. multi .. ",8,0,0")
    ECS.sendMessage("RenderEntityCommand", "SetColor:" .. multi .. ",0,0,1")
    table.insert(menuIDs, multi)

    -- Caméra du menu
    local menuCam = ECS.createEntity()
    ECS.addComponent(menuCam, "Transform", Transform(0, 0, 20, 0, 0, 0, 1, 1, 1))
    ECS.addComponent(menuCam, "Camera", Camera(true))
    table.insert(menuIDs, menuCam)
end

-- ============================================================================
-- HIDE MENU: Destruction du menu et remise à false du state
-- ============================================================================
function MenuSystem.hideMenu()
    for _, id in ipairs(menuIDs) do
        ECS.destroyEntity(id)
    end
    menuIDs = {}
    isMenuRendered = false
end

-- ============================================================================
-- MOUSE CLICK: Traitement des clics sur les cubes du menu
-- ============================================================================
function MenuSystem.onMousePressed(msg)
    if not isMenuRendered then return end -- silent guard

    local x = tonumber(msg:match(":(%d+),"))
    if not x then return end

    local choice = (x < 400) and "SOLO" or "MULTI"

    -- Update GameState
    -- REMOVED: Do NOT set state directly here. Let GameStateManager handle it via REQUEST_GAME_STATE_CHANGE
    -- so that it triggers the GAME_STARTED event for LevelManager/NetworkSystem.
    -- local gsEntities = ECS.getEntitiesWith({"GameState"})
    -- if #gsEntities > 0 then
    --    local gs = ECS.getComponent(gsEntities[1], "GameState")
    --    gs.state = "PLAYING"
    -- end

    if choice == "SOLO" then
        print("DEBUG: Step 1 - Hiding Menu")
        MenuSystem.hideMenu()
        print("DEBUG: Step 2 - Setting Game Mode")
        ECS.setGameMode("SOLO") -- Call the new C++ function
        
        print("DEBUG: Step 3 - Getting GameState entities")
        local gsEntities = ECS.getEntitiesWith({"GameState"})
        print("DEBUG: Step 4 - GameState count: " .. #gsEntities)

        -- Add Authority to GameState now that we are Solo/Server
        if #gsEntities > 0 then
             print("DEBUG: Step 5 - Adding ServerAuthority")
             -- Ensure ServerAuthority function exists
             if ServerAuthority then
                 local sa = ServerAuthority()
                 print("DEBUG: Step 5b - Created ServerAuthority component")
                 ECS.addComponent(gsEntities[1], "ServerAuthority", sa)
                 print("DEBUG: Step 5c - Added component")
             else
                 print("DEBUG: ERROR - ServerAuthority function is nil!")
             end
        end
        
        print("DEBUG: Step 6 - Requesting Game Start")
        print("[MenuSystem] Starting Solo Game via State Manager...")
        ECS.sendMessage("REQUEST_GAME_STATE_CHANGE", "PLAYING")

        print("DEBUG: Step 7 - Creating Camera")
        local gameCam = ECS.createEntity()
        ECS.addComponent(gameCam, "Transform", Transform(0, 0, 25, 0, 0, 0, 1, 1, 1))
        ECS.addComponent(gameCam, "Camera", Camera(true))
        
        -- Start background music when game begins
        ECS.sendMessage("MusicPlay", "bgm:music/background.ogg:40")
        print("DEBUG: Step 8 - Finished")

    elseif choice == "MULTI" then
        MenuSystem.hideMenu()
        ECS.setGameMode("MULTI_CLIENT") -- Call the new C++ function, assuming menu choice leads to client
        
        -- Start background music when game begins (client side)
        ECS.sendMessage("MusicPlay", "bgm:music/background.ogg:40")
        
        -- Signal server to start the game (payload must not be empty for topic parsing)
        print("[MenuSystem] Sending REQUEST_GAME_START to server...")
        ECS.sendNetworkMessage("REQUEST_GAME_START", "1")

        -- The actual network connection and whether it's a "server" or "client" for real
        -- will be determined by the network module and ECS.isServer()
        -- The network module will update ECS.capabilities again via the NetworkStatus message.
    end
end

-- ============================================================================
-- UPDATE: Animation des cubes du menu (Rotation)
-- ============================================================================
function MenuSystem.update(dt)
    if not isMenuRendered then return end

    for _, id in ipairs(menuIDs) do
        if ECS.hasComponent(id, "Mesh") then
            local t = ECS.getComponent(id, "Transform")
            if t then t.ry = t.ry + 100 * dt end
        end
    end
end

-- Register once; C++ will call MenuSystem.init() after this registration
ECS.registerSystem(MenuSystem)

return MenuSystem
