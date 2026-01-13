-- ============================================================================
-- MenuSystem.lua - 2D UI Menu System
-- ============================================================================
-- A proper 2D menu system using screen-space UI elements
-- ============================================================================

local MenuSystem = {}
local Spawns = require("assets/scripts/space-shooter/spawns")

-- ============================================================================
-- STATE
-- ============================================================================
local isMenuRendered = false
local menuElements = {}     -- All UI element IDs for cleanup
local menuButtons = {}      -- Button data for interaction
local selectedIndex = 1
local menuState = "MAIN"    -- MAIN, SETTINGS, PAUSE
local isPaused = false

-- Settings state (must be defined before executeAction uses it)
local settingsState = {
    isFullscreen = false,
    selectedResolution = 1,
    resolutions = {
        { width = 800, height = 600, label = "800x600" },
        { width = 1024, height = 768, label = "1024x768" },
        { width = 1280, height = 720, label = "1280x720" },
        { width = 1920, height = 1080, label = "1920x1080" },
    }
}

-- Screen dimensions (will be updated from renderer if possible)
local SCREEN_WIDTH = 800
local SCREEN_HEIGHT = 600

-- ============================================================================
-- COLORS
-- ============================================================================
local COLORS = {
    background = { r = 0.05, g = 0.05, b = 0.1, a = 0.95 },
    title = { r = 1.0, g = 0.8, b = 0.0 },
    buttonNormal = { r = 0.2, g = 0.2, b = 0.3, a = 0.9 },
    buttonSelected = { r = 0.1, g = 0.6, b = 0.3, a = 1.0 },
    textNormal = { r = 0.9, g = 0.9, b = 0.9 },
    textSelected = { r = 1.0, g = 1.0, b = 1.0 },
    solo = { r = 0.1, g = 0.5, b = 0.2, a = 0.9 },
    multi = { r = 0.1, g = 0.3, b = 0.6, a = 0.9 },
    settings = { r = 0.5, g = 0.3, b = 0.1, a = 0.9 },
    quit = { r = 0.5, g = 0.1, b = 0.1, a = 0.9 },
    resume = { r = 0.1, g = 0.5, b = 0.2, a = 0.9 }
}

-- ============================================================================
-- HELPER: Create a button with background and text
-- ============================================================================
function MenuSystem.createButton(action, text, x, y, width, height, color, textSize, zBase)
    textSize = textSize or 24
    zBase = zBase or 10
    
    -- Create rounded rectangle background for nicer looking buttons
    local bgId = ECS.createRoundedRect(x, y, width, height, 8, color.r, color.g, color.b, color.a or 0.9, zBase)
    table.insert(menuElements, bgId)
    
    -- Add subtle outline
    ECS.setOutline(bgId, true, 2, 0.3, 0.3, 0.4)
    
    -- Create text label (centered on button)
    local textX = x + width / 2 - (#text * textSize * 0.3)
    local textY = y + height / 2 - textSize / 2
    local textId = ECS.createUIText(text, textX, textY, textSize, 1, 1, 1, zBase + 1)
    table.insert(menuElements, textId)
    
    -- Store button data
    local buttonData = {
        action = action,
        bgId = bgId,
        textId = textId,
        x = x, y = y,
        width = width, height = height,
        baseColor = color,
        text = text
    }
    table.insert(menuButtons, buttonData)
    
    return buttonData
end

-- ============================================================================
-- HELPER: Create a text label
-- ============================================================================
function MenuSystem.createLabel(text, x, y, size, color, zOrder)
    zOrder = zOrder or 15
    local id = ECS.createUIText(text, x, y, size, color.r, color.g, color.b, zOrder)
    table.insert(menuElements, id)
    return id
end
-- ============================================================================
-- INIT
-- ============================================================================
function MenuSystem.init()
    print("[MenuSystem] Initialized (2D UI Mode)")
    ECS.subscribe("MousePressed", MenuSystem.onMousePressed)
    ECS.subscribe("KeyPressed", MenuSystem.onKeyPressed)
    ECS.subscribe("MouseMoved", MenuSystem.onMouseMoved)
    ECS.subscribe("PAUSE_GAME", MenuSystem.showPauseMenu)
    ECS.subscribe("RESUME_GAME", MenuSystem.hidePauseMenu)
    ECS.subscribe("WindowResized", MenuSystem.onWindowResized)

    -- Create GameState entity
    local gs = ECS.createEntity()
    ECS.addComponent(gs, "GameState", GameState("MENU", 0))
    if ECS.capabilities.hasAuthority then
        ECS.addComponent(gs, "ServerAuthority", ServerAuthority())
    end
    print("[MenuSystem] GameState entity created: " .. gs)

    -- Create camera for any 3D background elements
    local menuCam = ECS.createEntity()
    ECS.addComponent(menuCam, "Transform", Transform(0, 0, 20, 0, 0, 0, 1, 1, 1))
    ECS.addComponent(menuCam, "Camera", Camera(90))

    MenuSystem.renderMenu()
end

-- ============================================================================
-- IS PAUSED
-- ============================================================================
function MenuSystem.isPaused()
    return isPaused
end

-- ============================================================================
-- RENDER MAIN MENU
-- ============================================================================
function MenuSystem.renderMenu()
    if isMenuRendered then return end
    
    isMenuRendered = true
    menuElements = {}
    menuButtons = {}
    selectedIndex = 1
    menuState = "MAIN"
    
    -- Background overlay with rounded corners
    local bgId = ECS.createRoundedRect(20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40, 
        15, COLORS.background.r, COLORS.background.g, COLORS.background.b, COLORS.background.a, 0)
    table.insert(menuElements, bgId)
    ECS.setOutline(bgId, true, 3, 0.2, 0.4, 0.6)
    
    -- Decorative circles in corners
    local circleRadius = 30
    local circle1 = ECS.createCircle(60, SCREEN_HEIGHT - 60, circleRadius, 0.1, 0.3, 0.5, 0.5, 1)
    local circle2 = ECS.createCircle(SCREEN_WIDTH - 60, SCREEN_HEIGHT - 60, circleRadius, 0.1, 0.3, 0.5, 0.5, 1)
    local circle3 = ECS.createCircle(60, 60, circleRadius, 0.1, 0.3, 0.5, 0.5, 1)
    local circle4 = ECS.createCircle(SCREEN_WIDTH - 60, 60, circleRadius, 0.1, 0.3, 0.5, 0.5, 1)
    table.insert(menuElements, circle1)
    table.insert(menuElements, circle2)
    table.insert(menuElements, circle3)
    table.insert(menuElements, circle4)
    ECS.setOutline(circle1, true, 2, 0.2, 0.5, 0.8)
    ECS.setOutline(circle2, true, 2, 0.2, 0.5, 0.8)
    ECS.setOutline(circle3, true, 2, 0.2, 0.5, 0.8)
    ECS.setOutline(circle4, true, 2, 0.2, 0.5, 0.8)
    
    -- Decorative lines
    local line1 = ECS.createLine(100, SCREEN_HEIGHT - 150, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 150, 2, 0.3, 0.5, 0.7, 0.6, 2)
    local line2 = ECS.createLine(100, 100, SCREEN_WIDTH - 100, 100, 2, 0.3, 0.5, 0.7, 0.6, 2)
    table.insert(menuElements, line1)
    table.insert(menuElements, line2)
    
    -- Title
    MenuSystem.createLabel("R-TYPE", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT - 120, 64, COLORS.title, 20)
    MenuSystem.createLabel("CLONE", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT - 180, 32, COLORS.title, 20)
    
    -- Button dimensions
    local btnWidth = 200
    local btnHeight = 50
    local btnSpacing = 20
    local startY = SCREEN_HEIGHT - 280
    
    -- SOLO Button (left)
    MenuSystem.createButton("SOLO", "SOLO PLAY", 
        SCREEN_WIDTH/2 - btnWidth - 20, startY,
        btnWidth, btnHeight, COLORS.solo, 28, 10)
    
    -- MULTIPLAYER Button (right)
    MenuSystem.createButton("MULTI", "MULTIPLAYER", 
        SCREEN_WIDTH/2 + 20, startY,
        btnWidth, btnHeight, COLORS.multi, 28, 10)
    
    -- SETTINGS Button (center below)
    MenuSystem.createButton("SETTINGS", "SETTINGS", 
        SCREEN_WIDTH/2 - btnWidth/2, startY - btnHeight - btnSpacing,
        btnWidth, btnHeight, COLORS.settings, 24, 10)
    
    -- QUIT Button (center bottom)
    MenuSystem.createButton("QUIT", "QUIT", 
        SCREEN_WIDTH/2 - btnWidth/2, startY - (btnHeight + btnSpacing) * 2,
        btnWidth, btnHeight, COLORS.quit, 24, 10)
    
    -- Instructions
    MenuSystem.createLabel("Use Arrow Keys or Mouse to navigate", 
        SCREEN_WIDTH/2 - 180, 40, 16, COLORS.textNormal, 15)
    MenuSystem.createLabel("Press ENTER or Click to select", 
        SCREEN_WIDTH/2 - 150, 20, 16, COLORS.textNormal, 15)
    
    MenuSystem.updateSelection()
    print("[MenuSystem] Main menu rendered with " .. #menuButtons .. " buttons")
end

-- ============================================================================
-- UPDATE SELECTION VISUAL
-- ============================================================================
function MenuSystem.updateSelection()
    for i, btn in ipairs(menuButtons) do
        if i == selectedIndex then
            -- Highlighted state
            ECS.setUIColor(btn.bgId, COLORS.buttonSelected.r, COLORS.buttonSelected.g, COLORS.buttonSelected.b)
            ECS.setUIColor(btn.textId, COLORS.textSelected.r, COLORS.textSelected.g, COLORS.textSelected.b)
        else
            -- Normal state
            ECS.setUIColor(btn.bgId, btn.baseColor.r, btn.baseColor.g, btn.baseColor.b)
            ECS.setUIColor(btn.textId, COLORS.textNormal.r, COLORS.textNormal.g, COLORS.textNormal.b)
        end
    end
end

-- ============================================================================
-- HIDE MENU
-- ============================================================================
function MenuSystem.hideMenu()
    for _, id in ipairs(menuElements) do
        ECS.destroyUI(id)
    end
    menuElements = {}
    menuButtons = {}
    isMenuRendered = false
    selectedIndex = 1
end

-- ============================================================================
-- EXECUTE ACTION
-- ============================================================================
function MenuSystem.executeAction(action)
    print("[MenuSystem] Executing action: " .. action)
    
    local gsEntities = ECS.getEntitiesWith({"GameState"})
    
    if action == "SOLO" then
        if #gsEntities > 0 then
            local gs = ECS.getComponent(gsEntities[1], "GameState")
            gs.state = "PLAYING"
        end
        
        MenuSystem.hideMenu()
        ECS.setGameMode("SOLO")
        
        if #gsEntities > 0 then
            ECS.addComponent(gsEntities[1], "ServerAuthority", ServerAuthority())
        end
        
        ECS.isGameRunning = true
        Spawns.createPlayer(-8, 0, 0, nil)
        
        local gameCam = ECS.createEntity()
        ECS.addComponent(gameCam, "Transform", Transform(0, 0, 25, 0, 0, 0, 1, 1, 1))
        ECS.addComponent(gameCam, "Camera", Camera(90))
        
        ECS.sendMessage("MusicPlay", "bgm:music/background.ogg:40")
        
    elseif action == "MULTI" then
        if #gsEntities > 0 then
            local gs = ECS.getComponent(gsEntities[1], "GameState")
            gs.state = "PLAYING"
        end
        
        MenuSystem.hideMenu()
        ECS.setGameMode("MULTI_CLIENT")
        ECS.sendMessage("MusicPlay", "bgm:music/background.ogg:40")
        
        print("[MenuSystem] Sending REQUEST_GAME_START to server...")
        ECS.sendNetworkMessage("REQUEST_GAME_START", "1")
        
    elseif action == "SETTINGS" then
        MenuSystem.showSettings()
        
    elseif action == "PAUSE_SETTINGS" then
        MenuSystem.showSettings()
        menuState = "PAUSE_SETTINGS"
        
    elseif action == "TOGGLE_FULLSCREEN" then
        settingsState.isFullscreen = not settingsState.isFullscreen
        ECS.setFullscreen(settingsState.isFullscreen)
        print("[MenuSystem] Fullscreen: " .. tostring(settingsState.isFullscreen))
        MenuSystem.showSettings()
        
    elseif action == "RES_800x600" then
        settingsState.selectedResolution = 1
        ECS.setWindowSize(800, 600)
        SCREEN_WIDTH = 800
        SCREEN_HEIGHT = 600
        print("[MenuSystem] Resolution set to 800x600")
        MenuSystem.showSettings()
        
    elseif action == "RES_1024x768" then
        settingsState.selectedResolution = 2
        ECS.setWindowSize(1024, 768)
        SCREEN_WIDTH = 1024
        SCREEN_HEIGHT = 768
        print("[MenuSystem] Resolution set to 1024x768")
        MenuSystem.showSettings()
        
    elseif action == "RES_1280x720" then
        settingsState.selectedResolution = 3
        ECS.setWindowSize(1280, 720)
        SCREEN_WIDTH = 1280
        SCREEN_HEIGHT = 720
        print("[MenuSystem] Resolution set to 1280x720 HD")
        MenuSystem.showSettings()
        
    elseif action == "RES_1920x1080" then
        settingsState.selectedResolution = 4
        ECS.setWindowSize(1920, 1080)
        SCREEN_WIDTH = 1920
        SCREEN_HEIGHT = 1080
        print("[MenuSystem] Resolution set to 1920x1080 FHD")
        MenuSystem.showSettings()
        
    elseif action == "QUIT" then
        print("[MenuSystem] Quitting game...")
        ECS.sendMessage("ExitApplication", "")
        
    elseif action == "BACK" then
        MenuSystem.hideMenu()
        if menuState == "PAUSE_SETTINGS" then
            isMenuRendered = false
            MenuSystem.showPauseMenu()
        else
            MenuSystem.renderMenu()
        end
        
    elseif action == "RESUME" then
        MenuSystem.hidePauseMenu()
        
    elseif action == "QUIT_TO_MENU" then
        print("[MenuSystem] Returning to main menu...")
        ECS.isGameRunning = false
        isPaused = false
        
        if #gsEntities > 0 then
            local gs = ECS.getComponent(gsEntities[1], "GameState")
            gs.state = "MENU"
        end
        
        ECS.sendMessage("MusicStop", "bgm")
        
        -- Destroy game entities
        local allEntities = ECS.getEntitiesWith({"Transform"})
        for _, id in ipairs(allEntities) do
            if not ECS.hasComponent(id, "GameState") then
                ECS.destroyEntity(id)
            end
        end
        
        MenuSystem.hideMenu()
        MenuSystem.renderMenu()
    end
end

-- ============================================================================
-- SHOW SETTINGS
-- ============================================================================

function MenuSystem.showSettings()
    MenuSystem.hideMenu()
    isMenuRendered = true
    menuElements = {}
    menuButtons = {}
    selectedIndex = 1
    menuState = "SETTINGS"
    
    -- Background with rounded corners
    local bgId = ECS.createRoundedRect(20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40,
        15, COLORS.background.r, COLORS.background.g, COLORS.background.b, COLORS.background.a, 0)
    table.insert(menuElements, bgId)
    ECS.setOutline(bgId, true, 3, 0.2, 0.4, 0.6)
    
    -- Title
    MenuSystem.createLabel("SETTINGS", SCREEN_WIDTH/2 - 80, SCREEN_HEIGHT - 100, 48, COLORS.title, 20)
    
    -- Decorative line under title
    local line = ECS.createLine(100, SCREEN_HEIGHT - 130, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 130, 2, 0.3, 0.5, 0.7, 0.6, 2)
    table.insert(menuElements, line)
    
    -- ==================== DISPLAY SECTION ====================
    MenuSystem.createLabel("DISPLAY", SCREEN_WIDTH/2 - 50, SCREEN_HEIGHT - 165, 24, COLORS.title, 20)
    
    -- Fullscreen toggle button
    local fsLabel = settingsState.isFullscreen and "FULLSCREEN: ON" or "FULLSCREEN: OFF"
    local fsColor = settingsState.isFullscreen and COLORS.solo or COLORS.multi
    MenuSystem.createButton("TOGGLE_FULLSCREEN", fsLabel,
        SCREEN_WIDTH/2 - 140, SCREEN_HEIGHT - 210, 280, 40, fsColor, 20, 10)
    
    -- Resolution Section Label
    MenuSystem.createLabel("Window Size:", SCREEN_WIDTH/2 - 60, SCREEN_HEIGHT - 260, 20, COLORS.textNormal, 15)
    
    -- Resolution preset buttons - 2x2 grid
    local btnW = 130
    local btnH = 38
    local startX = SCREEN_WIDTH/2 - btnW - 10
    local startY = SCREEN_HEIGHT - 310
    local spacing = 10
    
    -- Row 1: 800x600 and 1024x768
    local res1Color = (settingsState.selectedResolution == 1) and COLORS.solo or COLORS.settings
    MenuSystem.createButton("RES_800x600", "800x600", 
        startX, startY, btnW, btnH, res1Color, 16, 10)
    
    local res2Color = (settingsState.selectedResolution == 2) and COLORS.solo or COLORS.settings
    MenuSystem.createButton("RES_1024x768", "1024x768", 
        startX + btnW + spacing, startY, btnW, btnH, res2Color, 16, 10)
    
    -- Row 2: 1280x720 and 1920x1080
    local res3Color = (settingsState.selectedResolution == 3) and COLORS.solo or COLORS.settings
    MenuSystem.createButton("RES_1280x720", "1280x720 HD", 
        startX, startY - btnH - spacing, btnW, btnH, res3Color, 14, 10)
    
    local res4Color = (settingsState.selectedResolution == 4) and COLORS.solo or COLORS.settings
    MenuSystem.createButton("RES_1920x1080", "1920x1080 FHD", 
        startX + btnW + spacing, startY - btnH - spacing, btnW, btnH, res4Color, 14, 10)
    
    -- Separator line
    local line2 = ECS.createLine(100, SCREEN_HEIGHT - 400, SCREEN_WIDTH - 100, SCREEN_HEIGHT - 400, 1, 0.3, 0.3, 0.4, 0.5, 2)
    table.insert(menuElements, line2)
    
    -- ==================== AUDIO SECTION ====================
    MenuSystem.createLabel("AUDIO", SCREEN_WIDTH/2 - 40, SCREEN_HEIGHT - 430, 24, COLORS.title, 20)
    
    MenuSystem.createLabel("Music: 40%  |  SFX: 60%", SCREEN_WIDTH/2 - 95, SCREEN_HEIGHT - 465, 16, COLORS.textNormal, 15)
    MenuSystem.createLabel("(Volume controls coming soon)", SCREEN_WIDTH/2 - 115, SCREEN_HEIGHT - 490, 14, { r = 0.5, g = 0.5, b = 0.5 }, 15)
    
    -- ==================== BACK BUTTON ====================
    MenuSystem.createButton("BACK", "BACK",
        SCREEN_WIDTH/2 - 100, 50, 200, 45, COLORS.quit, 22, 10)
    
    MenuSystem.updateSelection()
end

-- ============================================================================
-- SHOW PAUSE MENU
-- ============================================================================
function MenuSystem.showPauseMenu()
    if isPaused or not ECS.isGameRunning then return end
    
    print("[MenuSystem] Showing pause menu")
    isPaused = true
    isMenuRendered = true
    menuElements = {}
    menuButtons = {}
    selectedIndex = 1
    menuState = "PAUSE"
    
    -- Semi-transparent overlay
    local bgId = ECS.createRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 0.7, 50)
    table.insert(menuElements, bgId)
    
    -- Pause panel background
    local panelW, panelH = 300, 350
    local panelX = SCREEN_WIDTH/2 - panelW/2
    local panelY = SCREEN_HEIGHT/2 - panelH/2
    local panelBg = ECS.createRect(panelX, panelY, panelW, panelH, 0.1, 0.1, 0.15, 0.95, 51)
    table.insert(menuElements, panelBg)
    
    -- Title
    MenuSystem.createLabel("PAUSED", SCREEN_WIDTH/2 - 60, panelY + panelH - 60, 40, COLORS.title, 55)
    
    -- Buttons
    local btnWidth = 200
    local btnHeight = 45
    local btnX = SCREEN_WIDTH/2 - btnWidth/2
    local startY = panelY + panelH - 120
    
    MenuSystem.createButton("RESUME", "RESUME", btnX, startY, btnWidth, btnHeight, COLORS.resume, 24, 52)
    MenuSystem.createButton("PAUSE_SETTINGS", "SETTINGS", btnX, startY - 60, btnWidth, btnHeight, COLORS.settings, 24, 52)
    MenuSystem.createButton("QUIT_TO_MENU", "MAIN MENU", btnX, startY - 120, btnWidth, btnHeight, COLORS.quit, 24, 52)
    
    MenuSystem.updateSelection()
    ECS.sendMessage("GAME_PAUSED", "")
end

-- ============================================================================
-- HIDE PAUSE MENU
-- ============================================================================
function MenuSystem.hidePauseMenu()
    if not isPaused then return end
    
    print("[MenuSystem] Hiding pause menu")
    isPaused = false
    MenuSystem.hideMenu()
    ECS.sendMessage("GAME_RESUMED", "")
end

-- ============================================================================
-- KEYBOARD INPUT
-- ============================================================================
function MenuSystem.onKeyPressed(key)
    -- F11 for fullscreen toggle (works anywhere)
    if key == "F11" then
        settingsState.isFullscreen = not settingsState.isFullscreen
        ECS.toggleFullscreen()
        print("[MenuSystem] Toggled fullscreen via F11")
        return
    end
    
    -- Handle ESC for pause
    if key == "ESCAPE" then
        if ECS.isGameRunning and not isMenuRendered then
            MenuSystem.showPauseMenu()
            return
        elseif isPaused then
            MenuSystem.hidePauseMenu()
            return
        elseif menuState == "SETTINGS" or menuState == "PAUSE_SETTINGS" then
            MenuSystem.executeAction("BACK")
            return
        elseif menuState == "MAIN" then
            MenuSystem.executeAction("QUIT")
            return
        end
    end
    
    if not isMenuRendered then return end
    
    if key == "UP" or key == "Z" or key == "W" then
        selectedIndex = selectedIndex - 1
        if selectedIndex < 1 then selectedIndex = #menuButtons end
        MenuSystem.updateSelection()
        
    elseif key == "DOWN" or key == "S" then
        selectedIndex = selectedIndex + 1
        if selectedIndex > #menuButtons then selectedIndex = 1 end
        MenuSystem.updateSelection()
        
    elseif key == "LEFT" or key == "Q" or key == "A" then
        if menuState == "MAIN" and selectedIndex == 2 then
            selectedIndex = 1
            MenuSystem.updateSelection()
        end
        
    elseif key == "RIGHT" or key == "D" then
        if menuState == "MAIN" and selectedIndex == 1 then
            selectedIndex = 2
            MenuSystem.updateSelection()
        end
        
    elseif key == "ENTER" or key == "SPACE" then
        if #menuButtons >= selectedIndex then
            MenuSystem.executeAction(menuButtons[selectedIndex].action)
        end
    end
end

-- ============================================================================
-- MOUSE INPUT
-- ============================================================================
function MenuSystem.onMouseMoved(msg)
    if not isMenuRendered then return end
    
    local x, y = msg:match("(%d+),(%d+)")
    if not x or not y then return end
    x, y = tonumber(x), tonumber(y)
    
    -- Convert to bottom-left origin (OpenGL style)
    y = SCREEN_HEIGHT - y
    
    for i, btn in ipairs(menuButtons) do
        if x >= btn.x and x <= btn.x + btn.width and
           y >= btn.y and y <= btn.y + btn.height then
            if selectedIndex ~= i then
                selectedIndex = i
                MenuSystem.updateSelection()
            end
            return
        end
    end
end

function MenuSystem.onMousePressed(msg)
    if not isMenuRendered then return end
    
    local btn, x, y = msg:match("(%d+):(%d+),(%d+)")
    if not x or not y then return end
    x, y = tonumber(x), tonumber(y)
    
    -- Convert to bottom-left origin (OpenGL style)
    local screenY = SCREEN_HEIGHT - y
    
    for i, button in ipairs(menuButtons) do
        if x >= button.x and x <= button.x + button.width and
           screenY >= button.y and screenY <= button.y + button.height then
            MenuSystem.executeAction(button.action)
            return
        end
    end
end

-- Handle window resize events from the engine
function MenuSystem.onWindowResized(msg)
    local w, h = msg:match("(%d+),(%d+)")
    if w and h then
        local newWidth = tonumber(w)
        local newHeight = tonumber(h)
        if newWidth and newHeight and newWidth > 0 and newHeight > 0 then
            print("[MenuSystem] Window resized to " .. newWidth .. "x" .. newHeight)
            SCREEN_WIDTH = newWidth
            SCREEN_HEIGHT = newHeight
            
            -- Update settings state to reflect actual resolution
            for i, res in ipairs(settingsState.resolutions) do
                if res.width == newWidth and res.height == newHeight then
                    settingsState.selectedResolution = i
                    break
                end
            end
            
            -- If menu is currently shown, redraw it with new dimensions
            if isMenuRendered then
                if menuState == "SETTINGS" or menuState == "PAUSE_SETTINGS" then
                    MenuSystem.showSettings()
                elseif menuState == "PAUSE" then
                    MenuSystem.showPauseMenu()
                elseif menuState == "MAIN" then
                    MenuSystem.renderMenu()
                end
            end
        end
    end
end

-- ============================================================================
-- UPDATE (for animations if needed)
-- ============================================================================
function MenuSystem.update(dt)
    -- Could add button hover animations, etc.
end

-- Register system
ECS.registerSystem(MenuSystem)

return MenuSystem
