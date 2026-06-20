-- ============================================================================
-- MenuSystem.lua - 2D UI Menu System
-- ============================================================================
-- A proper 2D menu system using screen-space UI elements
-- ============================================================================

local MenuSystem = {}
local Spawns = require("assets/scripts/space-shooter/spawns")
local ScoreSystem = require("assets/scripts/space-shooter/systems/ScoreSystem")

-- ============================================================================
-- STATE
-- ============================================================================
local isMenuRendered = false
local menuElements = {}     -- All UI element IDs for cleanup
local menuButtons = {}      -- Button data for interaction
local selectedIndex = 1
local menuState = "MAIN"    -- MAIN, MULTIPLAYER, SETTINGS, PAUSE
local isPaused = false
local levelIntro = {
    active = false,
    timer = 0,
    duration = 2.2,
    elementIds = {}
}

ECS.isPaused = false

-- Settings state (must be defined before executeAction uses it)
local settingsState = {
    isFullscreen = false,
    uiScaleIndex = 2,
    uiScales = {
        { value = 0.85, label = "85%" },
        { value = 1.00, label = "100%" },
        { value = 1.15, label = "115%" },
        { value = 1.30, label = "130%" },
    },
    highContrast = false,
    largeText = false,
    reducedMotion = false,
    musicVolume = 40,
    sfxVolume = 70
}
local deathScreen = {
    active = false,
    score = 0
}
ECS.settings = settingsState
ECS.getSfxVolume = function(defaultVolume)
    local base = tonumber(defaultVolume) or 100
    return math.floor(base * (settingsState.sfxVolume / 100) + 0.5)
end

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
    solo = { r = 0.5, g = 0.0, b = 0.5, a = 0.9 },
    multi = { r = 0.1, g = 0.3, b = 0.6, a = 0.9 },
    settings = { r = 0.5, g = 0.3, b = 0.1, a = 0.9 },
    quit = { r = 0.5, g = 0.1, b = 0.1, a = 0.9 },
    resume = { r = 0.1, g = 0.5, b = 0.2, a = 0.9 }
}

local DEFAULT_COLORS = COLORS
local HIGH_CONTRAST_COLORS = {
    background = { r = 0.0, g = 0.0, b = 0.0, a = 0.96 },
    title = { r = 1.0, g = 0.95, b = 0.15 },
    buttonNormal = { r = 0.05, g = 0.05, b = 0.08, a = 0.98 },
    buttonSelected = { r = 0.0, g = 0.45, b = 1.0, a = 1.0 },
    textNormal = { r = 1.0, g = 1.0, b = 1.0 },
    textSelected = { r = 1.0, g = 1.0, b = 0.2 },
    solo = { r = 0.0, g = 0.25, b = 0.8, a = 0.95 },
    multi = { r = 0.0, g = 0.45, b = 0.75, a = 0.95 },
    settings = { r = 0.18, g = 0.18, b = 0.18, a = 0.95 },
    quit = { r = 0.65, g = 0.0, b = 0.0, a = 0.95 },
    resume = { r = 0.0, g = 0.55, b = 0.16, a = 0.95 }
}

local uiScaleOverride = nil

local function getUiScale()
    if uiScaleOverride then
        return uiScaleOverride
    end
    local scale = settingsState.uiScales[settingsState.uiScaleIndex]
    local base = scale and scale.value or 1.0
    if settingsState.largeText then
        base = base + 0.15
    end
    return base
end

local function ui(value)
    return math.floor(value * getUiScale() + 0.5)
end

local function uiWithScale(value, scale)
    return math.floor(value * scale + 0.5)
end

local function applyAccessibilityPalette()
    COLORS = settingsState.highContrast and HIGH_CONTRAST_COLORS or DEFAULT_COLORS
end

local function applyAudioSettings()
    ECS.sendMessage("MusicSetVolume", "bgm:" .. tostring(settingsState.musicVolume))
end

local function formatToggle(enabled)
    return enabled and "ON" or "OFF"
end

local function cycleIndex(current, list, delta)
    local nextIndex = current + delta
    if nextIndex < 1 then
        nextIndex = #list
    elseif nextIndex > #list then
        nextIndex = 1
    end
    return nextIndex
end

-- ============================================================================
-- HELPER: Estimate text width for centering calculations
-- ============================================================================
-- For Arial font, average character width is approximately 0.52 * fontSize
-- This accounts for variable-width characters in the font
local CHAR_WIDTH_RATIO = 0.6

local function estimateTextWidth(text, fontSize)
    return #text * fontSize * CHAR_WIDTH_RATIO
end

-- Helper to create a centered label at a given Y position
local function createCenteredLabel(text, y, size, color, zOrder)
    local displaySize = ui(size)
    local textWidth = estimateTextWidth(text, displaySize)
    local x = (SCREEN_WIDTH - textWidth) / 2 + 40
    return MenuSystem.createLabel(text, x, y, size, color, zOrder)
end

-- ============================================================================
-- HELPER: Create a button with background and text
-- ============================================================================
function MenuSystem.createButton(action, text, x, y, width, height, color, textSize, zBase)
    textSize = ui(textSize or 24)
    zBase = zBase or 10
    local radius = math.max(8, ui(10))

    -- Create rounded rectangle background for nicer looking buttons
    local bgId = ECS.createRoundedRect(x, y, width, height, radius, color.r, color.g, color.b, color.a or 0.9, zBase)
    table.insert(menuElements, bgId)

    ECS.setOutline(bgId, true, settingsState.highContrast and 4 or 2, 0.45, 0.65, 0.9)

    -- Create text label (centered on button)
    local textWidth = estimateTextWidth(text, textSize)
    local textX = x + (width - textWidth) / 2
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
    local id = ECS.createUIText(text, x, y, ui(size), color.r, color.g, color.b, zOrder)
    table.insert(menuElements, id)
    return id
end
-- ============================================================================
-- INIT
-- ============================================================================
function MenuSystem.init()
    print("[MenuSystem] Initialized (2D UI Mode)")
    _G.MenuSystem = MenuSystem
    applyAccessibilityPalette()
    ECS.subscribe("MousePressed", MenuSystem.onMousePressed)
    ECS.subscribe("KeyPressed", MenuSystem.onKeyPressed)
    ECS.subscribe("MouseMoved", MenuSystem.onMouseMoved)
    ECS.subscribe("PAUSE_GAME", MenuSystem.showPauseMenu)
    ECS.subscribe("RESUME_GAME", MenuSystem.hidePauseMenu)
    ECS.subscribe("WindowResized", MenuSystem.onWindowResized)
    ECS.subscribe("SET_GAME_MODE", MenuSystem.onSetGameMode)
    ECS.subscribe("GAME_START", MenuSystem.onGameStart)
    ECS.subscribe("GAME_STARTING", MenuSystem.onGameStarting)
    ECS.subscribe("GAME_END", MenuSystem.onGameEnd)
    ECS.subscribe("GAME_OVER", MenuSystem.onGameOver)
    ECS.subscribe("FORCE_WAITING_ROOM", MenuSystem.onForceWaitingRoom)
    ECS.subscribe("ShowLevelIntro", MenuSystem.onShowLevelIntro)
    _G.SCREEN_WIDTH = SCREEN_WIDTH
    _G.SCREEN_HEIGHT = SCREEN_HEIGHT

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

local function clearLevelIntro()
    if not levelIntro.active then
        return
    end
    for _, id in ipairs(levelIntro.elementIds) do
        ECS.destroyUI(id)
    end
    levelIntro.elementIds = {}
    levelIntro.timer = 0
    levelIntro.active = false
end

function MenuSystem.onShowLevelIntro(msg)
    if not ECS.capabilities.hasRendering then
        return
    end

    clearLevelIntro()

    local level = tonumber(msg) or (_G.CurrentLevel or 1)
    local centerX = SCREEN_WIDTH / 2
    local centerY = SCREEN_HEIGHT / 2

    local panelW = math.min(560, SCREEN_WIDTH - 80)
    local panelH = 170
    local panelX = centerX - panelW / 2
    local panelY = centerY - panelH / 2

    local panelId = ECS.createRoundedRect(panelX, panelY, panelW, panelH,
        14, 0.04, 0.08, 0.16, 0.92, 80)
    ECS.setOutline(panelId, true, 3, 0.3, 0.6, 0.95)
    table.insert(levelIntro.elementIds, panelId)

    local levelTitle = "LEVEL " .. tostring(level)
    local titleWidth = estimateTextWidth(levelTitle, 52)
    local titleId = ECS.createUIText(levelTitle, centerX - titleWidth / 2, centerY + 18,
        52, 1.0, 0.9, 0.2, 81)
    table.insert(levelIntro.elementIds, titleId)

    local subtitle = "Destroy the giant boss to clear this level"
    local subWidth = estimateTextWidth(subtitle, 18)
    local subId = ECS.createUIText(subtitle, centerX - subWidth / 2, centerY - 36,
        18, 0.88, 0.92, 1.0, 81)
    table.insert(levelIntro.elementIds, subId)

    levelIntro.active = true
    levelIntro.timer = 0
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

    applyAccessibilityPalette()

    local bgId = ECS.createRoundedRect(20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40,
        ui(15), COLORS.background.r, COLORS.background.g, COLORS.background.b, COLORS.background.a, 0)
    table.insert(menuElements, bgId)
    ECS.setOutline(bgId, true, settingsState.highContrast and 4 or 3, 0.25, 0.55, 0.9)

    if not settingsState.reducedMotion then
        local circleRadius = ui(28)
        local circle1 = ECS.createCircle(70, SCREEN_HEIGHT - 70, circleRadius, 0.1, 0.3, 0.5, 0.45, 1)
        local circle2 = ECS.createCircle(SCREEN_WIDTH - 70, 70, circleRadius, 0.1, 0.3, 0.5, 0.45, 1)
        table.insert(menuElements, circle1)
        table.insert(menuElements, circle2)
        ECS.setOutline(circle1, true, 2, 0.2, 0.5, 0.8)
        ECS.setOutline(circle2, true, 2, 0.2, 0.5, 0.8)
    end

    local line1 = ECS.createLine(90, SCREEN_HEIGHT - 155, SCREEN_WIDTH - 90, SCREEN_HEIGHT - 155, 2, 0.3, 0.5, 0.7, 0.7, 2)
    local line2 = ECS.createLine(90, 105, SCREEN_WIDTH - 90, 105, 2, 0.3, 0.5, 0.7, 0.7, 2)
    table.insert(menuElements, line1)
    table.insert(menuElements, line2)

    -- Title (centered)
    local textWidthR = estimateTextWidth("R-TYPE", ui(64))
    local xR = (SCREEN_WIDTH - textWidthR) / 2
    MenuSystem.createLabel("R-TYPE", xR, SCREEN_HEIGHT - 135, 64, COLORS.title, 20)
    local textWidthC = estimateTextWidth("CLONE", ui(32))
    local xC = (SCREEN_WIDTH - textWidthC) / 2
    MenuSystem.createLabel("CLONE", xC, SCREEN_HEIGHT - 185, 32, COLORS.title, 20)
    
    -- Button dimensions
    local btnWidth = math.min(ui(300), SCREEN_WIDTH - 120)
    local btnHeight = ui(50)
    local btnSpacing = ui(24)
    local startY = SCREEN_HEIGHT - 280

    local twoColumn = SCREEN_WIDTH >= 760 and getUiScale() <= 1.15
    if twoColumn then
        MenuSystem.createButton("SOLO", "SOLO PLAY",
            SCREEN_WIDTH/2 - btnWidth - ui(24), startY,
            btnWidth, btnHeight, COLORS.solo, 28, 10)
        MenuSystem.createButton("MULTI", "MULTIPLAYER",
            SCREEN_WIDTH/2 + ui(24), startY,
            btnWidth, btnHeight, COLORS.multi, 28, 10)
    else
        MenuSystem.createButton("SOLO", "SOLO PLAY",
            SCREEN_WIDTH/2 - btnWidth/2, startY,
            btnWidth, btnHeight, COLORS.solo, 28, 10)
        MenuSystem.createButton("MULTI", "MULTIPLAYER",
            SCREEN_WIDTH/2 - btnWidth/2, startY - btnHeight - btnSpacing,
            btnWidth, btnHeight, COLORS.multi, 28, 10)
        startY = startY - btnHeight - btnSpacing
    end

    -- SETTINGS Button (center below)
    MenuSystem.createButton("SETTINGS", "SETTINGS",
        SCREEN_WIDTH/2 - btnWidth/2, startY - btnHeight - btnSpacing,
        btnWidth, btnHeight, COLORS.settings, 24, 10)

    -- QUIT Button (center bottom)
    MenuSystem.createButton("QUIT", "QUIT",
        SCREEN_WIDTH/2 - btnWidth/2, startY - (btnHeight + btnSpacing) * 2,
        btnWidth, btnHeight, COLORS.quit, 24, 10)

    -- Instructions (centered)
    createCenteredLabel("Arrow keys, WASD/ZQSD, mouse, Enter or Space", 62, 16, COLORS.textNormal, 15)
    createCenteredLabel("Esc pauses, goes back, or quits from the main menu", 40, 16, COLORS.textNormal, 15)
    
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
            ECS.setOutline(btn.bgId, true, settingsState.highContrast and 5 or 3, COLORS.textSelected.r, COLORS.textSelected.g, COLORS.textSelected.b)
        else
            -- Normal state
            ECS.setUIColor(btn.bgId, btn.baseColor.r, btn.baseColor.g, btn.baseColor.b)
            ECS.setUIColor(btn.textId, COLORS.textNormal.r, COLORS.textNormal.g, COLORS.textNormal.b)
            ECS.setOutline(btn.bgId, true, settingsState.highContrast and 3 or 2, 0.35, 0.4, 0.55)
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
    
    -- Helper function for clean start
    local function cleanStart()
        ECS.isGameRunning = false
        isPaused = false
        ECS.isPaused = false
        ECS.timeScale = 1.0
        ECS.deathSlowdownActive = false
        deathScreen.active = false
        _G.LevelBossActive = false
        _G.LevelBossDefeated = {}
        ECS.sendMessage("RESET_BOSS_STATE", "")
        ECS.sendMessage("MusicStop", "bgm")
        -- Destroy game entities
        local allEntities = ECS.getEntitiesWith({"Transform"})
        for _, id in ipairs(allEntities) do
            if not ECS.hasComponent(id, "GameState") then
                ECS.sendMessage("PhysicCommand", "DestroyBody:" .. id .. ";")
                ECS.destroyEntity(id)
            end
        end
    end
    
    local gsEntities = ECS.getEntitiesWith({"GameState"})

    if action == "SOLO" then
        cleanStart()
        if #gsEntities > 0 then
            local gs = ECS.getComponent(gsEntities[1], "GameState")
            gs.state = "PLAYING"
        end

        MenuSystem.hideMenu()
        ECS.setGameMode("SOLO")
        ECS.sendMessage("MusicPlay", "bgm:music/background.ogg:" .. tostring(settingsState.musicVolume))
        
        if #gsEntities > 0 then
            ECS.addComponent(gsEntities[1], "ServerAuthority", ServerAuthority())
        end
        ECS.isGameRunning = true

        Spawns.createPlayer(-8, 0, 0, nil)
        local level = 1
        _G.CurrentLevel = 1

        local file = io.open("current_level.txt", "w")
        if file then
            file:write("1")
            file:close()
        end

        print("[MenuSystem] Loading Level " .. level .. " for solo mode")
        dofile("assets/scripts/space-shooter/levels/Level-" .. level .. ".lua")

        ScoreSystem.adjustToScreenSize(SCREEN_WIDTH, SCREEN_HEIGHT)

    elseif action == "MULTI" then
        cleanStart()
        ECS.setGameMode("MULTI_CLIENT")
        MenuSystem.showMultiplayerMenu()
        ECS.sendNetworkMessage("PLAYER_JOIN", "join")

    elseif action == "READY" then
        ECS.sendNetworkMessage("PLAYER_READY", "ready")
        print("[MenuSystem] Player marked ready, waiting for GAME_START")

    elseif action == "START_FROM_LOBBY" then
        ECS.sendNetworkMessage("PLAYER_READY", "ready")

    elseif action == "SETTINGS" then
        MenuSystem.showSettings()

    elseif action == "PAUSE_SETTINGS" then
        MenuSystem.showSettings()
        menuState = "PAUSE_SETTINGS"

    elseif action == "UI_SCALE_PREV" then
        settingsState.uiScaleIndex = cycleIndex(settingsState.uiScaleIndex, settingsState.uiScales, -1)
        print("[MenuSystem] UI scale set to " .. settingsState.uiScales[settingsState.uiScaleIndex].label)
        MenuSystem.showSettings()

    elseif action == "UI_SCALE_NEXT" then
        settingsState.uiScaleIndex = cycleIndex(settingsState.uiScaleIndex, settingsState.uiScales, 1)
        print("[MenuSystem] UI scale set to " .. settingsState.uiScales[settingsState.uiScaleIndex].label)
        MenuSystem.showSettings()

    elseif action == "MUSIC_DOWN" then
        settingsState.musicVolume = math.max(0, settingsState.musicVolume - 10)
        applyAudioSettings()
        print("[MenuSystem] Music volume set to " .. settingsState.musicVolume)
        MenuSystem.showSettings()

    elseif action == "MUSIC_UP" then
        settingsState.musicVolume = math.min(100, settingsState.musicVolume + 10)
        applyAudioSettings()
        print("[MenuSystem] Music volume set to " .. settingsState.musicVolume)
        MenuSystem.showSettings()

    elseif action == "SFX_DOWN" then
        settingsState.sfxVolume = math.max(0, settingsState.sfxVolume - 10)
        ECS.sendMessage("SoundSetVolume", "ui_select:" .. tostring(settingsState.sfxVolume))
        print("[MenuSystem] SFX volume set to " .. settingsState.sfxVolume)
        MenuSystem.showSettings()

    elseif action == "SFX_UP" then
        settingsState.sfxVolume = math.min(100, settingsState.sfxVolume + 10)
        ECS.sendMessage("SoundPlay", "ui_select:effects/powerup.wav:" .. tostring(settingsState.sfxVolume))
        print("[MenuSystem] SFX volume set to " .. settingsState.sfxVolume)
        MenuSystem.showSettings()

    elseif action == "TOGGLE_CONTRAST" then
        settingsState.highContrast = not settingsState.highContrast
        applyAccessibilityPalette()
        print("[MenuSystem] High contrast " .. formatToggle(settingsState.highContrast))
        MenuSystem.showSettings()

    elseif action == "TOGGLE_LARGE_TEXT" then
        settingsState.largeText = not settingsState.largeText
        print("[MenuSystem] Large text " .. formatToggle(settingsState.largeText))
        MenuSystem.showSettings()

    elseif action == "TOGGLE_REDUCED_MOTION" then
        settingsState.reducedMotion = not settingsState.reducedMotion
        print("[MenuSystem] Reduced motion " .. formatToggle(settingsState.reducedMotion))
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
        ECS.isPaused = false
        ECS.timeScale = 1.0
        ECS.deathSlowdownActive = false
        deathScreen.active = false
        _G.LevelBossActive = false
        _G.LevelBossDefeated = {}
        ECS.sendMessage("RESET_BOSS_STATE", "")

        if #gsEntities > 0 then
            local gs = ECS.getComponent(gsEntities[1], "GameState")
            gs.state = "MENU"
        end

        ECS.sendMessage("MusicStop", "bgm")

        if ECS.capabilities.hasNetworkSync and not ECS.capabilities.hasAuthority then
            -- Multiplayer client: leave active match and go back to waiting room.
            ECS.sendNetworkMessage("PLAYER_LEAVE", "menu")
        end

        -- Destroy game entities
        local allEntities = ECS.getEntitiesWith({"Transform"})
        for _, id in ipairs(allEntities) do
            if not ECS.hasComponent(id, "GameState") then
                ECS.destroyEntity(id)
            end
        end

        MenuSystem.hideMenu()
        if ECS.capabilities.hasNetworkSync and not ECS.capabilities.hasAuthority then
            MenuSystem.showMultiplayerMenu()
        else
            MenuSystem.renderMenu()
        end
    elseif action == "RESTART_GAME" then
        deathScreen.active = false
        ECS.timeScale = 1.0
        ECS.deathSlowdownActive = false
        MenuSystem.hideMenu()
        MenuSystem.executeAction("SOLO")
    end
end

function MenuSystem.showDeathScreen(finalScore)
    if not ECS.capabilities.hasRendering then return end

    MenuSystem.hideMenu()
    clearLevelIntro()
    isMenuRendered = true
    menuElements = {}
    menuButtons = {}
    selectedIndex = 1
    menuState = "DEATH"
    deathScreen.active = true
    deathScreen.score = tonumber(finalScore) or 0

    local bgId = ECS.createRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0.65, 0.0, 0.0, 0.5, 70)
    table.insert(menuElements, bgId)

    local panelW = math.min(ui(520), SCREEN_WIDTH - ui(80))
    local panelH = ui(270)
    local panelX = SCREEN_WIDTH / 2 - panelW / 2
    local panelY = SCREEN_HEIGHT / 2 - panelH / 2
    local panel = ECS.createRoundedRect(panelX, panelY, panelW, panelH, ui(16), 0.05, 0.01, 0.01, 0.72, 71)
    table.insert(menuElements, panel)
    ECS.setOutline(panel, true, 4, 1.0, 0.15, 0.1)

    local title = "GAME OVER"
    local titleSize = 56
    local titleW = estimateTextWidth(title, ui(titleSize))
    MenuSystem.createLabel(title, SCREEN_WIDTH / 2 - titleW / 2, panelY + panelH - ui(80), titleSize, { r = 1.0, g = 0.12, b = 0.08 }, 72)

    local scoreText = "Final Score: " .. tostring(deathScreen.score)
    local scoreW = estimateTextWidth(scoreText, ui(22))
    MenuSystem.createLabel(scoreText, SCREEN_WIDTH / 2 - scoreW / 2, panelY + panelH - ui(120), 22, COLORS.textNormal, 72)

    local btnW = math.min(ui(210), panelW - ui(60))
    local btnH = ui(46)
    local gap = ui(18)
    local btnY = panelY + ui(56)

    if panelW >= ui(480) then
        MenuSystem.createButton("RESTART_GAME", "RESTART", SCREEN_WIDTH / 2 - btnW - gap / 2, btnY, btnW, btnH, COLORS.resume, 22, 72)
        MenuSystem.createButton("QUIT_TO_MENU", "MENU", SCREEN_WIDTH / 2 + gap / 2, btnY, btnW, btnH, COLORS.quit, 22, 72)
    else
        MenuSystem.createButton("RESTART_GAME", "RESTART", SCREEN_WIDTH / 2 - btnW / 2, btnY + btnH + gap, btnW, btnH, COLORS.resume, 22, 72)
        MenuSystem.createButton("QUIT_TO_MENU", "MENU", SCREEN_WIDTH / 2 - btnW / 2, btnY, btnW, btnH, COLORS.quit, 22, 72)
    end

    MenuSystem.updateSelection()
end

function MenuSystem.onGameOver(score)
    MenuSystem.showDeathScreen(score)
end

function MenuSystem.showMultiplayerMenu()
    MenuSystem.hideMenu()
    isMenuRendered = true
    menuElements = {}
    menuButtons = {}
    selectedIndex = 1
    menuState = "MULTIPLAYER"

    local bgId = ECS.createRoundedRect(20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40,
        15, COLORS.background.r, COLORS.background.g, COLORS.background.b, COLORS.background.a, 0)
    table.insert(menuElements, bgId)
    ECS.setOutline(bgId, true, 3, 0.2, 0.4, 0.6)

    createCenteredLabel("MULTIPLAYER LOBBY", SCREEN_HEIGHT - 120, 42, COLORS.title, 20)
    createCenteredLabel("Connect all clients, then press READY", SCREEN_HEIGHT - 170, 18, COLORS.textNormal, 20)
    createCenteredLabel("Game starts automatically when everyone is ready", SCREEN_HEIGHT - 200, 16, COLORS.textNormal, 20)

    local btnWidth = 260
    local btnHeight = 50
    MenuSystem.createButton("READY", "READY", SCREEN_WIDTH / 2 - btnWidth / 2, SCREEN_HEIGHT / 2 - 20,
        btnWidth, btnHeight, COLORS.multi, 26, 10)
    MenuSystem.createButton("BACK", "BACK", SCREEN_WIDTH / 2 - btnWidth / 2, SCREEN_HEIGHT / 2 - 90,
        btnWidth, btnHeight, COLORS.quit, 24, 10)

    MenuSystem.updateSelection()
end

function MenuSystem.onGameStart(_)
    local gsEntities = ECS.getEntitiesWith({"GameState"})
    if #gsEntities > 0 then
        local gs = ECS.getComponent(gsEntities[1], "GameState")
        gs.state = "PLAYING"
    end

    if ECS.capabilities.hasRendering and isMenuRendered then
        MenuSystem.hideMenu()
    end

    if ECS.capabilities.hasRendering then
        ECS.sendMessage("MusicPlay", "bgm:music/background.ogg:" .. tostring(settingsState.musicVolume))
    end
    ECS.isGameRunning = true

    local currentLevel = _G.CurrentLevel or 1
    MenuSystem.onShowLevelIntro(tostring(currentLevel))
end

function MenuSystem.onForceWaitingRoom(_)
    ECS.isGameRunning = false
    isPaused = false
    ECS.isPaused = false
    MenuSystem.hideMenu()
    MenuSystem.showMultiplayerMenu()
    print("[MenuSystem] Switched to waiting room")
end

function MenuSystem.onGameStarting(_)
    -- Game is about to start - could show a "Starting..." message
    print("[MenuSystem] Game starting...")
    if ECS.capabilities.hasRendering and isMenuRendered then
        -- Optional: show "Game Starting..." overlay
    end
end

function MenuSystem.onGameEnd(_)
    -- Game ended - return to lobby
    print("[MenuSystem] Game ended - returning to lobby")
    ECS.isGameRunning = false
    isPaused = false
    ECS.isPaused = false

    local gsEntities = ECS.getEntitiesWith({"GameState"})
    if #gsEntities > 0 then
        local gs = ECS.getComponent(gsEntities[1], "GameState")
        gs.state = "MENU"
    end

    ECS.sendMessage("MusicStop", "bgm")

    -- Cleanup game entities
    local allEntities = ECS.getEntitiesWith({"Transform"})
    for _, id in ipairs(allEntities) do
        if not ECS.hasComponent(id, "GameState") and not ECS.hasComponent(id, "Camera") then
            local tag = ECS.getComponent(id, "Tag")
            local keep = false
            if tag then
                for _, t in ipairs(tag.tags) do
                    if t == "MenuEntity" or t == "GameUI" then
                        keep = true
                        break
                    end
                end
            end
            if not keep then
                ECS.destroyEntity(id)
            end
        end
    end

    MenuSystem.hideMenu()

    -- If in multiplayer mode, go back to lobby
    if ECS.capabilities.hasNetworkSync and not ECS.capabilities.hasAuthority then
        MenuSystem.showMultiplayerMenu()
    else
        MenuSystem.renderMenu()
    end
end

-- ============================================================================
-- SHOW SETTINGS
-- ============================================================================

function MenuSystem.showSettings()
    MenuSystem.hideMenu()
    applyAccessibilityPalette()
    isMenuRendered = true
    menuElements = {}
    menuButtons = {}
    selectedIndex = 1
    if menuState ~= "PAUSE_SETTINGS" then
        menuState = "SETTINGS"
    end

    local bgId = ECS.createRoundedRect(20, 20, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 40,
        ui(15), COLORS.background.r, COLORS.background.g, COLORS.background.b, COLORS.background.a, 0)
    table.insert(menuElements, bgId)
    ECS.setOutline(bgId, true, settingsState.highContrast and 4 or 3, 0.25, 0.55, 0.9)

    local compactWidth = SCREEN_WIDTH < ui(760)
    local compactHeight = SCREEN_HEIGHT < ui(640)
    local topY = SCREEN_HEIGHT - (compactHeight and ui(70) or ui(92))

    createCenteredLabel("SETTINGS", topY, compactHeight and 40 or 48, COLORS.title, 20)

    local lineY = topY - ui(28)
    local line = ECS.createLine(ui(60), lineY, SCREEN_WIDTH - ui(60), lineY, 2, 0.3, 0.5, 0.7, 0.6, 2)
    table.insert(menuElements, line)

    local settingsScale = getUiScale()
    if compactHeight then
        settingsScale = math.min(settingsScale, compactWidth and 0.82 or 0.92)
    elseif compactWidth then
        settingsScale = math.min(settingsScale, 0.95)
    end
    uiScaleOverride = settingsScale
    local function su(value)
        return uiWithScale(value, settingsScale)
    end

    local btnH = su(compactHeight and 32 or 38)
    local btnW = su(compactWidth and 72 or 48)
    local rowGap = compactWidth and su(compactHeight and 68 or 78) or su(60)
    local rowY = lineY - su(compactHeight and 62 or 76)
    local contentW = math.min(SCREEN_WIDTH - ui(80), ui(620))
    local contentX = (SCREEN_WIDTH - contentW) / 2
    local labelX = contentX
    local valueW = su(compactWidth and 138 or 96)
    local valueX = compactWidth and (contentX + contentW - valueW) or (SCREEN_WIDTH / 2 + ui(12))
    local leftBtnX = valueX - btnW - su(8)
    local rightBtnX = valueX + valueW + su(8)
    local compactControlX = contentX + math.max(0, (contentW - (btnW * 2 + valueW + su(16))) / 2)

    local function settingRow(label, value, y, leftAction, rightAction)
        local valueText = tostring(value)
        if compactWidth then
            MenuSystem.createLabel(label, labelX, y + btnH + su(13), 15, COLORS.textNormal, 15)
            MenuSystem.createLabel(valueText, valueX, y + btnH + su(13), 15, COLORS.title, 15)
            local cx = compactControlX
            if leftAction then
                MenuSystem.createButton(leftAction, "<", cx, y, btnW, btnH, COLORS.settings, 22, 10)
            end
            if rightAction then
                MenuSystem.createButton(rightAction, ">", cx + btnW + valueW + su(16), y, btnW, btnH, COLORS.settings, 22, 10)
            end
        else
            MenuSystem.createLabel(label, labelX, y + su(9), 16, COLORS.textNormal, 15)
            MenuSystem.createLabel(valueText, valueX, y + su(9), 16, COLORS.title, 15)
            if leftAction then
                MenuSystem.createButton(leftAction, "<", leftBtnX, y, btnW, btnH, COLORS.settings, 22, 10)
            end
            if rightAction then
                MenuSystem.createButton(rightAction, ">", rightBtnX, y, btnW, btnH, COLORS.settings, 22, 10)
            end
        end
    end

    local function toggleRow(label, enabled, y, action)
        local color = enabled and COLORS.solo or COLORS.settings
        if compactWidth then
            MenuSystem.createLabel(label, labelX, y + btnH + su(13), 15, COLORS.textNormal, 15)
            MenuSystem.createButton(action, formatToggle(enabled), compactControlX + btnW + ui(8), y, valueW, btnH, color, 16, 10)
        else
            MenuSystem.createLabel(label, labelX, y + su(9), 16, COLORS.textNormal, 15)
            MenuSystem.createButton(action, formatToggle(enabled), valueX, y, valueW, btnH, color, 18, 10)
        end
    end

    if compactWidth then
        settingRow("UI Scale", settingsState.uiScales[settingsState.uiScaleIndex].label,
            rowY, "UI_SCALE_PREV", "UI_SCALE_NEXT")
        settingRow("Music Volume", tostring(settingsState.musicVolume) .. "%",
            rowY - rowGap, "MUSIC_DOWN", "MUSIC_UP")
        settingRow("SFX Volume", tostring(settingsState.sfxVolume) .. "%",
            rowY - rowGap * 2, "SFX_DOWN", "SFX_UP")
        toggleRow("High Contrast", settingsState.highContrast, rowY - rowGap * 3, "TOGGLE_CONTRAST")
        toggleRow("Large Text", settingsState.largeText, rowY - rowGap * 4, "TOGGLE_LARGE_TEXT")
        toggleRow("Reduced Motion", settingsState.reducedMotion, rowY - rowGap * 5, "TOGGLE_REDUCED_MOTION")
    else
        local colGap = su(44)
        local colW = (contentW - colGap) / 2
        local leftColX = contentX
        local rightColX = contentX + colW + colGap
        local colValueW = su(86)
        local colBtnW = su(42)

        local function settingRowAt(label, value, y, colX, leftAction, rightAction)
            local vX = colX + su(150)
            MenuSystem.createLabel(label, colX, y + su(9), 16, COLORS.textNormal, 15)
            MenuSystem.createLabel(tostring(value), vX, y + su(9), 16, COLORS.title, 15)
            if leftAction then
                MenuSystem.createButton(leftAction, "<", vX - colBtnW - su(8), y, colBtnW, btnH, COLORS.settings, 20, 10)
            end
            if rightAction then
                MenuSystem.createButton(rightAction, ">", vX + colValueW + su(8), y, colBtnW, btnH, COLORS.settings, 20, 10)
            end
        end

        local function toggleRowAt(label, enabled, y, colX, action)
            local color = enabled and COLORS.solo or COLORS.settings
            MenuSystem.createLabel(label, colX, y + su(9), 16, COLORS.textNormal, 15)
            MenuSystem.createButton(action, formatToggle(enabled), colX + su(150), y, colValueW, btnH, color, 16, 10)
        end

        MenuSystem.createLabel("DISPLAY / AUDIO", leftColX, rowY + su(48), 20, COLORS.title, 20)
        MenuSystem.createLabel("ACCESSIBILITY", rightColX, rowY + su(48), 20, COLORS.title, 20)
        settingRowAt("UI Scale", settingsState.uiScales[settingsState.uiScaleIndex].label,
            rowY, leftColX, "UI_SCALE_PREV", "UI_SCALE_NEXT")
        settingRowAt("Music", tostring(settingsState.musicVolume) .. "%",
            rowY - rowGap, leftColX, "MUSIC_DOWN", "MUSIC_UP")
        settingRowAt("SFX", tostring(settingsState.sfxVolume) .. "%",
            rowY - rowGap * 2, leftColX, "SFX_DOWN", "SFX_UP")
        toggleRowAt("High Contrast", settingsState.highContrast, rowY, rightColX, "TOGGLE_CONTRAST")
        toggleRowAt("Large Text", settingsState.largeText, rowY - rowGap, rightColX, "TOGGLE_LARGE_TEXT")
        toggleRowAt("Reduced Motion", settingsState.reducedMotion, rowY - rowGap * 2, rightColX, "TOGGLE_REDUCED_MOTION")
    end

    local hint = compactWidth and "Resize manually. F11 fullscreen." or "Window size follows manual resizing. Fullscreen: F11."
    createCenteredLabel(hint, compactHeight and 76 or 82, 15, COLORS.textNormal, 15)

    MenuSystem.createButton("BACK", "BACK",
        SCREEN_WIDTH/2 - ui(100), 35, ui(200), ui(45), COLORS.quit, 22, 10)

    uiScaleOverride = nil
    MenuSystem.updateSelection()
end

-- ============================================================================
-- SHOW PAUSE MENU
-- ============================================================================
function MenuSystem.showPauseMenu()
    print("[MenuSystem] Showing pause menu")
    isPaused = true
    ECS.isPaused = true -- Global pause flag for other systems
    isMenuRendered = true
    menuElements = {}
    menuButtons = {}
    selectedIndex = 1
    menuState = "PAUSE"

    local bgId = ECS.createRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, settingsState.highContrast and 0.86 or 0.7, 50)
    table.insert(menuElements, bgId)

    local panelW, panelH = ui(320), ui(350)
    local panelX = SCREEN_WIDTH/2 - panelW/2
    local panelY = SCREEN_HEIGHT/2 - panelH/2
    local panelBg = ECS.createRoundedRect(panelX, panelY, panelW, panelH, ui(14), COLORS.background.r, COLORS.background.g, COLORS.background.b, 0.96, 51)
    table.insert(menuElements, panelBg)
    ECS.setOutline(panelBg, true, settingsState.highContrast and 4 or 3, 0.35, 0.65, 0.95)

    local textWidth = estimateTextWidth("PAUSED", ui(40))
    local titleX = SCREEN_WIDTH/2 - textWidth/2
    MenuSystem.createLabel("PAUSED", titleX, panelY + panelH - ui(60), 40, COLORS.title, 55)
    
    local btnWidth = math.min(ui(250), panelW - ui(36))
    local btnHeight = ui(45)
    local btnX = SCREEN_WIDTH/2 - btnWidth/2
    local startY = panelY + panelH - ui(130)
    
    MenuSystem.createButton("RESUME", "RESUME", btnX, startY, btnWidth, btnHeight, COLORS.resume, 24, 52)
    MenuSystem.createButton("PAUSE_SETTINGS", "SETTINGS", btnX, startY - ui(70), btnWidth, btnHeight, COLORS.settings, 24, 52)
    MenuSystem.createButton("QUIT_TO_MENU", "MAIN MENU", btnX, startY - ui(140), btnWidth, btnHeight, COLORS.quit, 24, 52)
    
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
    ECS.isPaused = false -- Global pause flag
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
            _G.SCREEN_WIDTH = newWidth
            _G.SCREEN_HEIGHT = newHeight

            -- If menu is currently shown, redraw it with new dimensions
            if isMenuRendered then
                if menuState == "SETTINGS" or menuState == "PAUSE_SETTINGS" then
                    MenuSystem.showSettings()
                elseif menuState == "PAUSE" then
                    MenuSystem.hideMenu()
                    MenuSystem.showPauseMenu()
                elseif menuState == "MAIN" then
                    MenuSystem.hideMenu()
                    MenuSystem.renderMenu()
                elseif menuState == "DEATH" then
                    MenuSystem.showDeathScreen(deathScreen.score)
                end
            end
        end
    end
end

-- ============================================================================
-- SET GAME MODE (from client)
-- ============================================================================
function MenuSystem.onSetGameMode(mode)
    print("[MenuSystem] Setting game mode to: " .. mode)
    ECS.setGameMode(mode)
end

-- ============================================================================
-- UPDATE (for animations if needed)
-- ============================================================================
function MenuSystem.update(dt)
    if ECS.isGameRunning
        and not deathScreen.active
        and ECS.capabilities.hasAuthority
        and ECS.capabilities.hasRendering
        and not ECS.capabilities.hasNetworkSync then
        local players = ECS.getEntitiesWith({"Player", "Life"})
        local shouldShowDeath = (#players == 0)
        if not shouldShowDeath then
            local life = ECS.getComponent(players[1], "Life")
            shouldShowDeath = life and (life.amount or 0) <= 0
        end
        if shouldShowDeath then
            local score = 0
            local scoreEntities = ECS.getEntitiesWith({"Score"})
            if #scoreEntities > 0 then
                local scoreComp = ECS.getComponent(scoreEntities[1], "Score")
                if scoreComp and scoreComp.value then
                    score = scoreComp.value
                end
            end
            ECS.isGameRunning = false
            ECS.deathSlowdownActive = true
            ECS.timeScale = 1.0
            MenuSystem.showDeathScreen(score)
        end
    end

    if levelIntro.active then
        levelIntro.timer = levelIntro.timer + dt
        if levelIntro.timer >= levelIntro.duration then
            clearLevelIntro()
        end
    end
end

-- Register system
ECS.registerSystem(MenuSystem)

return MenuSystem
