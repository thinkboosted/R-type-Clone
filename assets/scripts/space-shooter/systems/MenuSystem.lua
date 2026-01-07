local MenuSystem = {}

local isMenuRendered = false
local menuIDs = {}

function MenuSystem.init()
    print("[MenuSystem] Initialized")
    ECS.subscribe("MousePressed", MenuSystem.onMousePressed)

    local gs = ECS.createEntity()
    ECS.addComponent(gs, "GameState", GameState("MENU", 0))
    if ECS.capabilities.hasAuthority then
        ECS.addComponent(gs, "ServerAuthority", ServerAuthority())
    end
    MenuSystem.renderMenu()
end

function MenuSystem.renderMenu()
    if isMenuRendered then return end
    isMenuRendered = true

    local solo = ECS.createEntity()
    ECS.addComponent(solo, "Transform", Transform(-8, 0, 0, 0, 0, 0, 2, 2, 2))
    ECS.addComponent(solo, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(solo, "Color", Color(0, 1, 0))
    table.insert(menuIDs, solo)

    local multi = ECS.createEntity()
    ECS.addComponent(multi, "Transform", Transform(8, 0, 0, 0, 0, 0, 2, 2, 2))
    ECS.addComponent(multi, "Mesh", Mesh("assets/models/cube.obj"))
    ECS.addComponent(multi, "Color", Color(0, 0, 1))
    table.insert(menuIDs, multi)
end

function MenuSystem.hideMenu()
    for _, id in ipairs(menuIDs) do ECS.destroyEntity(id) end
    menuIDs = {}
    isMenuRendered = false
end

function MenuSystem.onMousePressed(msg)
    if not isMenuRendered then return end
    local x = tonumber(msg:match(":(%d+),"))
    if not x then return end

    local choice = (x < 400) and "SOLO" or "MULTI"

    if choice == "SOLO" then
        MenuSystem.hideMenu()
        ECS.setGameMode("SOLO")
        print("[MenuSystem] Starting Solo Session...")
        ECS.sendMessage("REQUEST_GAME_STATE_CHANGE", "PLAYING")
    elseif choice == "MULTI" then
        MenuSystem.hideMenu()
        ECS.setGameMode("MULTI_CLIENT")
        print("[MenuSystem] Connecting to Multi...")
        ECS.sendNetworkMessage("REQUEST_GAME_START", "1")
    end
end

function MenuSystem.update(dt)
    if not isMenuRendered then return end
    for _, id in ipairs(menuIDs) do
        local t = ECS.getComponent(id, "Transform")
        if t then t.ry = t.ry + 100 * dt end
    end
end

ECS.registerSystem(MenuSystem)
return MenuSystem