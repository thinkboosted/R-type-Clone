local MenuSystem = {}

MenuSystem.gameState = "MENU" -- MENU, PLAYING, GAMEOVER
MenuSystem.lastScore = 0
MenuSystem.spawnTimer = 0

function MenuSystem.init()
    print("[MenuSystem] Initialized")
    
    ECS.subscribe("GAME_OVER", function(msg)
        local score = tonumber(msg) or 0
        MenuSystem.lastScore = score
        MenuSystem.setState("GAMEOVER")
    end)

    ECS.subscribe("KeyPressed", function(msg)
        if MenuSystem.gameState == "MENU" and msg == "SPACE" then
            print("Space Pressed -> Playing")
            MenuSystem.setState("PLAYING")
            MenuSystem.requestSpawn()
        elseif MenuSystem.gameState == "GAMEOVER" and msg == "SPACE" then
            MenuSystem.setState("MENU")
        end
    end)
    
    -- Initial State
    MenuSystem.createMenu()
end

function MenuSystem.createMenu()
    print("Creating Menu 3D UI...")
    
    -- Green Cube as PLAY Button (Center of Screen)
    local btn = ECS.createEntity()
    ECS.addComponent(btn, "Transform", Transform(0, 0, 0)) -- World Center
    ECS.addComponent(btn, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(btn, "Color", Color(0.0, 1.0, 0.0)) -- Green
    ECS.addComponent(btn, "Button", Button("PLAY", 200, 200)) -- Logical size
    ECS.addComponent(btn, "Tag", Tag({"MenuEntity"}))
    
    -- Visual indication (Rotation/Scale handled by RenderSystem via Transform)
    local t = ECS.getComponent(btn, "Transform")
    t.sx = 2.0; t.sy = 2.0; t.sz = 2.0
end

function MenuSystem.createGameOver()
    print("Creating GameOver 3D UI...")
    
    -- Red Cube as MENU Button
    local btn = ECS.createEntity()
    ECS.addComponent(btn, "Transform", Transform(0, 0, 0))
    ECS.addComponent(btn, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(btn, "Color", Color(1.0, 0.0, 0.0)) -- Red
    ECS.addComponent(btn, "Button", Button("MENU", 200, 200))
    ECS.addComponent(btn, "Tag", Tag({"GameOverEntity"}))
end

function MenuSystem.setState(newState)
    MenuSystem.gameState = newState
    
    -- Clear UI
    local menuEnts = ECS.getEntitiesWith({"Tag"})
    for _, id in ipairs(menuEnts) do
        local tag = ECS.getComponent(id, "Tag")
        for _, t in ipairs(tag.tags) do
            if t == "MenuEntity" or t == "GameOverEntity" then
                ECS.destroyEntity(id)
                break
            end
        end
    end

    if newState == "MENU" then
        MenuSystem.createMenu()
    elseif newState == "GAMEOVER" then
        MenuSystem.createGameOver()
    elseif newState == "PLAYING" then
        -- Game UI?
    end
end

function MenuSystem.requestSpawn()
    ECS.sendNetworkMessage("REQUEST_SPAWN", "")
    MenuSystem.spawnTimer = 0.0
end

function MenuSystem.onMousePressed(msg)
    local btnCode, x, y = string.match(msg, "(%d+)%s+(%d+)%s+(%d+)")
    if not btnCode then
        btnCode, x, y = string.match(msg, "(%d+):(%d+):(%d+)")
    end
    if not btnCode then
         -- Format seen: "0:407,245"
         btnCode, x, y = string.match(msg, "(%d+):(%d+),(%d+)")
    end

    x = tonumber(x)
    y = tonumber(y)

    if not x or not y then return end
    
    print("Click at: " .. x .. "," .. y)

    -- Define Center Zone (assuming 800x600 window)
    -- Center is 400, 300.
    -- Cube is roughly in the middle. Let's give a 100px radius.
    local centerX = 400
    local centerY = 300
    local radius = 100
    
    if x > (centerX - radius) and x < (centerX + radius) and
       y > (centerY - radius) and y < (centerY + radius) then
       
       print("Hit Center Button!")
       
       if MenuSystem.gameState == "MENU" then
           MenuSystem.setState("PLAYING")
           MenuSystem.requestSpawn()
       elseif MenuSystem.gameState == "GAMEOVER" then
           MenuSystem.setState("MENU")
       end
    end
end

function MenuSystem.update(dt)
    if MenuSystem.gameState == "PLAYING" then
        MenuSystem.spawnTimer = MenuSystem.spawnTimer + dt
        if MenuSystem.spawnTimer > 1.0 and MenuSystem.spawnTimer < 5.0 then
             ECS.sendNetworkMessage("REQUEST_SPAWN", "")
             MenuSystem.spawnTimer = 0.0
        end
    end
    
    -- Rotate Menu Cubes
    local menuEnts = ECS.getEntitiesWith({"Tag", "Transform"})
    for _, id in ipairs(menuEnts) do
        local t = ECS.getComponent(id, "Transform")
        t.rx = t.rx + 50 * dt
        t.ry = t.ry + 50 * dt
    end
end

ECS.registerSystem(MenuSystem)