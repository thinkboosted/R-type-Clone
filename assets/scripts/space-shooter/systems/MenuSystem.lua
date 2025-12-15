local MenuSystem = {}

MenuSystem.gameState = "MENU"
MenuSystem.lastScore = 0
MenuSystem.spawnTimer = 0
MenuSystem.connectionTimeout = 0
MenuSystem.assignmentHandled = false

function MenuSystem.init()
    print("MENU: Initialized")

    ECS.subscribe("GAME_OVER", function(msg)
        print("MENU: Received GAME_OVER")
        local score = tonumber(msg) or 0
        MenuSystem.lastScore = score
        MenuSystem.setState("GAMEOVER")
    end)

    ECS.subscribe("NetworkError", function(msg)
        print("MENU: Network Error intercepted: " .. msg)
        MenuSystem.lastError = msg -- Store for potential display if needed
        MenuSystem.setState("ERROR")
    end)

    MenuSystem.createMenu()
end

function MenuSystem.createMenu()
    print("MENU: Creating Menu UI...")

    -- SOLO Button (Left, Green)
    local soloBtn = ECS.createEntity()
    ECS.addComponent(soloBtn, "Transform", Transform(-8, 0, 0))
    ECS.addComponent(soloBtn, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(soloBtn, "Color", Color(0.0, 1.0, 0.0))
    ECS.addComponent(soloBtn, "Button", Button("SOLO", 200, 200))
    ECS.addComponent(soloBtn, "Tag", Tag({"MenuEntity"}))
    local t1 = ECS.getComponent(soloBtn, "Transform")
    t1.sx = 2.0; t1.sy = 2.0; t1.sz = 2.0

    -- MULTI Button (Right, Blue)
    local multiBtn = ECS.createEntity()
    ECS.addComponent(multiBtn, "Transform", Transform(8, 0, 0))
    ECS.addComponent(multiBtn, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(multiBtn, "Color", Color(0.0, 0.0, 1.0))
    ECS.addComponent(multiBtn, "Button", Button("MULTI", 200, 200))
    ECS.addComponent(multiBtn, "Tag", Tag({"MenuEntity"}))
    local t2 = ECS.getComponent(multiBtn, "Transform")
    t2.sx = 2.0; t2.sy = 2.0; t2.sz = 2.0
end

function MenuSystem.createGameOver()
    print("MENU: Creating GameOver UI...")
    local btn = ECS.createEntity()
    ECS.addComponent(btn, "Transform", Transform(0, 0, 0))
    ECS.addComponent(btn, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(btn, "Color", Color(1.0, 0.0, 0.0))
    ECS.addComponent(btn, "Button", Button("MENU", 200, 200))
    ECS.addComponent(btn, "Tag", Tag({"GameOverEntity"}))
end

function MenuSystem.createErrorMenu()
    print("MENU: Creating Error UI...")
    local btn = ECS.createEntity()
    ECS.addComponent(btn, "Transform", Transform(0, 0, 0))
    ECS.addComponent(btn, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(btn, "Color", Color(1.0, 0.5, 0.0)) -- Orange
    ECS.addComponent(btn, "Button", Button("BACK", 200, 200))
    ECS.addComponent(btn, "Tag", Tag({"ErrorEntity"}))

    local txt = ECS.createEntity()
    ECS.addComponent(txt, "Transform", Transform(200, 100, 0))
    ECS.addComponent(txt, "Text", Text("CONNECTION FAILED", "assets/fonts/arial.ttf", 36, true))
    ECS.addComponent(txt, "Color", Color(1.0, 0.0, 0.0))
    ECS.addComponent(txt, "Tag", Tag({"ErrorEntity"}))
end

function MenuSystem.createGameUI()
    print("MENU: Creating Game UI...")
    local scoreEntity = ECS.createEntity()
    ECS.addComponent(scoreEntity, "Score", Score(0))
    ECS.addComponent(scoreEntity, "Transform", Transform(650, 550, 0))
    ECS.addComponent(scoreEntity, "Text", Text("Score: 0", "assets/fonts/arial.ttf", 24, true))
    ECS.addComponent(scoreEntity, "Color", Color(1.0, 1.0, 1.0))
    ECS.addComponent(scoreEntity, "Tag", Tag({"GameUI"}))

    if not ECS.isLocal then
        print("MENU: Creating Loading Cube...")
        local loading = ECS.createEntity()
        ECS.addComponent(loading, "Transform", Transform(0, 0, 0))
        ECS.addComponent(loading, "Mesh", Mesh("assets/models/cube.obj"))
        ECS.addComponent(loading, "Color", Color(1.0, 1.0, 0.0)) -- Yellow
        ECS.addComponent(loading, "Tag", Tag({"GameUI", "LoadingEntity"}))
        local t = ECS.getComponent(loading, "Transform")
        t.sx = 0.5; t.sy = 0.5; t.sz = 0.5
    end
end

function MenuSystem.setState(newState)
    print("MENU: Changing state from " .. MenuSystem.gameState .. " to " .. newState)
    MenuSystem.gameState = newState

    local entsToDestroy = {}
    local menuEnts = ECS.getEntitiesWith({"Tag"})
    for _, id in ipairs(menuEnts) do
        local tag = ECS.getComponent(id, "Tag")
        for _, t in ipairs(tag.tags) do
            if t == "MenuEntity" or t == "GameOverEntity" or t == "GameUI" or t == "ErrorEntity" or t == "LoadingEntity" then
                table.insert(entsToDestroy, id)
                break
            end
        end
    end
    for _, id in ipairs(entsToDestroy) do
        ECS.destroyEntity(id)
    end

    if newState == "MENU" then
        ECS.isGameRunning = false
        ECS.sendMessage("CLIENT_RESET", "")
        MenuSystem.assignmentHandled = false
        MenuSystem.createMenu()
    elseif newState == "GAMEOVER" then
        ECS.isGameRunning = false
        ECS.sendMessage("CLIENT_RESET", "")
        MenuSystem.assignmentHandled = false
        MenuSystem.createGameOver()
    elseif newState == "ERROR" then
        ECS.isGameRunning = false
        ECS.sendMessage("CLIENT_RESET", "")
        MenuSystem.assignmentHandled = false
        MenuSystem.createErrorMenu()
    elseif newState == "PLAYING" then
        MenuSystem.assignmentHandled = false
        MenuSystem.createGameUI()
    end
end

function MenuSystem.startSolo()
    print("MENU: Starting SOLO Mode...")
    ECS.isLocal = true
    ECS.isGameRunning = true

    local player = ECS.createEntity()
    ECS.addComponent(player, "Transform", Transform(-8, 0, 0, 0, 0, -90))
    ECS.addComponent(player, "Mesh", Mesh("assets/models/aircraft.obj"))
    ECS.addComponent(player, "Collider", Collider("Box", {1, 1, 1}))
    ECS.addComponent(player, "Physic", Physic(1.0, 0.0, true, false))
    ECS.addComponent(player, "Player", Player(20.0))
    ECS.addComponent(player, "Weapon", Weapon(0.2))
    ECS.addComponent(player, "Life", { amount = 3, max = 3, invulnerableTime = 1.0 })
    ECS.addComponent(player, "Color", Color(0.0, 1.0, 0.0))
    ECS.addComponent(player, "InputState", { up=false, down=false, left=false, right=false, shoot=false })

    MenuSystem.setState("PLAYING")
end

function MenuSystem.startMulti()
    print("MENU: Starting MULTI Mode...")
    ECS.isLocal = false
    ECS.isGameRunning = true

    MenuSystem.spawnTimer = 0
    MenuSystem.connectionTimeout = 0

    ECS.sendMessage("RequestNetworkConnect", "127.0.0.1 4242")

    MenuSystem.setState("PLAYING")
end

function MenuSystem.onMousePressed(msg)
    local btnCode, x, y = string.match(msg, "(%d+)%s+(%d+)%s+(%d+)")
    if not btnCode then btnCode, x, y = string.match(msg, "(%d+):(%d+):(%d+)") end
    if not btnCode then btnCode, x, y = string.match(msg, "(%d+):(%d+),(%d+)") end

    x = tonumber(x)
    y = tonumber(y)
    if not x or not y then return end

    print("MENU: Click at: " .. x .. "," .. y .. ", current state: " .. MenuSystem.gameState)

    local width = 800

    if MenuSystem.gameState == "MENU" then
        if x < width / 2 then
            MenuSystem.startSolo()
        else
            MenuSystem.startMulti()
        end
    elseif MenuSystem.gameState == "GAMEOVER" or MenuSystem.gameState == "ERROR" then
        MenuSystem.setState("MENU")
    end
end

function MenuSystem.update(dt)
    if ECS.isServer() then return end

    if MenuSystem.gameState == "PLAYING" and not ECS.isLocal then
        -- Check if player assigned (NetworkSystem.myServerId is set)
        if NetworkSystem and NetworkSystem.myServerId then
            if not MenuSystem.assignmentHandled then
                local entsToDestroy = {}
                local loadingEnts = ECS.getEntitiesWith({"Tag"})
                for _, id in ipairs(loadingEnts) do
                    local tag = ECS.getComponent(id, "Tag")
                    for _, t in ipairs(tag.tags) do
                        if t == "LoadingEntity" then table.insert(entsToDestroy, id) end
                    end
                end
                for _, id in ipairs(entsToDestroy) do ECS.destroyEntity(id) end
                MenuSystem.connectionTimeout = 0
                MenuSystem.assignmentHandled = true
            end
            return -- Already assigned, nothing else to do in this update.
        end

        -- Waiting for assignment
        MenuSystem.spawnTimer = MenuSystem.spawnTimer + dt
        MenuSystem.connectionTimeout = MenuSystem.connectionTimeout + dt

        if MenuSystem.spawnTimer > 1.0 then
             ECS.sendNetworkMessage("REQUEST_SPAWN", "")
             MenuSystem.spawnTimer = 0.0
        end

        if MenuSystem.connectionTimeout > 5.0 then
            print("MENU: LOGIC: Connection Timeout triggered! Setting state to ERROR.")
            MenuSystem.setState("ERROR")
            MenuSystem.connectionTimeout = 0 -- Reset for next try
            return -- Exit to prevent further processing in this state
        end

        local ents = ECS.getEntitiesWith({"Tag", "Transform"})
        for _, id in ipairs(ents) do
            local tag = ECS.getComponent(id, "Tag")
            for _, t in ipairs(tag.tags) do
                if t == "LoadingEntity" then
                    local tr = ECS.getComponent(id, "Transform")
                    tr.rx = tr.rx + 100 * dt
                    tr.ry = tr.ry + 100 * dt
                end
            end
        end
    end

    local menuEnts = ECS.getEntitiesWith({"Tag", "Transform"})
    for _, id in ipairs(menuEnts) do
        local tag = ECS.getComponent(id, "Tag")
        local isMenu = false
        for _, t in ipairs(tag.tags) do if t == "MenuEntity" then isMenu = true end end

        if isMenu then
            local t = ECS.getComponent(id, "Transform")
            t.rx = t.rx + 50 * dt
            t.ry = t.ry + 50 * dt
        end
    end
end

ECS.registerSystem(MenuSystem)