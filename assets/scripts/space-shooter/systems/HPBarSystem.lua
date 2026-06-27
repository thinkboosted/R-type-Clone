local HPBarSystem = {}

HPBarSystem.bgId = nil
HPBarSystem.fillId = nil
HPBarSystem.textId = nil
HPBarSystem.maxWidth = 260
HPBarSystem.height = 24
HPBarSystem.lastHP = -1
HPBarSystem.lastMax = -1
HPBarSystem.bossBgId = nil
HPBarSystem.bossFillId = nil
HPBarSystem.bossTextId = nil
HPBarSystem.bossMaxWidth = 430
HPBarSystem.bossHeight = 24
HPBarSystem.lastBossId = nil
HPBarSystem.lastBossHP = -1
HPBarSystem.lastBossMax = -1

local function ensureUI()
    if HPBarSystem.bgId and HPBarSystem.fillId and HPBarSystem.textId then
        return
    end

    local x = 24
    local y = 64
    HPBarSystem.bgId = ECS.createRect(x, y, HPBarSystem.maxWidth, HPBarSystem.height, 0.12, 0.12, 0.12, 0.82, 70)
    HPBarSystem.fillId = ECS.createRect(x + 2, y + 2, HPBarSystem.maxWidth - 4, HPBarSystem.height - 4, 0.2, 0.85, 0.2, 0.95, 71)
    HPBarSystem.textId = ECS.createUIText("HP: 100/100", x + 6, y + 2, 16, 1.0, 1.0, 1.0, 72)
end

local function getLocalPlayerLife()
    -- Prefer explicitly local player in network mode.
    if ECS.capabilities.hasNetworkSync and _G.NetworkSystem and _G.NetworkSystem.myServerId and _G.NetworkSystem.serverEntities then
        local localId = _G.NetworkSystem.serverEntities[_G.NetworkSystem.myServerId]
        if localId and ECS.hasComponent(localId, "Life") then
            local life = ECS.getComponent(localId, "Life")
            if life then
                return life.amount or 0, life.max or 100
            end
        end
    end

    local players = ECS.getEntitiesWith({"Player", "Life"})
    if #players > 0 then
        local life = ECS.getComponent(players[1], "Life")
        if life then
            return life.amount or 0, life.max or 100
        end
    end

    return nil, nil
end

local function getBossLife()
    local bosses = ECS.getEntitiesWith({"Boss", "Life"})
    if #bosses == 0 then
        return nil, nil, nil
    end

    local bossId = bosses[1]
    local life = ECS.getComponent(bossId, "Life")
    if not life then
        return nil, nil, nil
    end

    return bossId, life.amount or 0, life.max or 1
end

local function ensureBossUI()
    if HPBarSystem.bossBgId and HPBarSystem.bossFillId and HPBarSystem.bossTextId then
        return
    end

    local screenW = _G.SCREEN_WIDTH or 800
    local screenH = _G.SCREEN_HEIGHT or 600
    local width = math.min(HPBarSystem.bossMaxWidth, screenW - 120)
    local x = (screenW - width) / 2
    local y = screenH - 42

    HPBarSystem.bossBgId = ECS.createRect(x, y, width, HPBarSystem.bossHeight, 0.18, 0.02, 0.02, 0.88, 73)
    HPBarSystem.bossFillId = ECS.createRect(x + 2, y + 2, width - 4, HPBarSystem.bossHeight - 4, 0.9, 0.1, 0.08, 0.95, 74)
    HPBarSystem.bossTextId = ECS.createUIText("BOSS", x + width / 2 - 34, y + 2, 16, 1.0, 1.0, 1.0, 75)
end

local function hideBossUI()
    if HPBarSystem.bossBgId then ECS.destroyUI(HPBarSystem.bossBgId) end
    if HPBarSystem.bossFillId then ECS.destroyUI(HPBarSystem.bossFillId) end
    if HPBarSystem.bossTextId then ECS.destroyUI(HPBarSystem.bossTextId) end
    HPBarSystem.bossBgId = nil
    HPBarSystem.bossFillId = nil
    HPBarSystem.bossTextId = nil
    HPBarSystem.lastBossId = nil
    HPBarSystem.lastBossHP = -1
    HPBarSystem.lastBossMax = -1
end

local function hpColor(ratio)
    if ratio > 0.6 then return 0.2, 0.85, 0.2 end
    if ratio > 0.3 then return 0.95, 0.75, 0.15 end
    return 0.9, 0.2, 0.2
end

function HPBarSystem.init()
    print("[HPBarSystem] Initialized")
end

function HPBarSystem.update(dt)
    if not ECS.capabilities.hasRendering then return end

    if not ECS.isGameRunning then
        if HPBarSystem.bgId then
            ECS.destroyUI(HPBarSystem.bgId)
            HPBarSystem.bgId = nil
        end
        if HPBarSystem.fillId then
            ECS.destroyUI(HPBarSystem.fillId)
            HPBarSystem.fillId = nil
        end
        if HPBarSystem.textId then
            ECS.destroyUI(HPBarSystem.textId)
            HPBarSystem.textId = nil
        end
        hideBossUI()
        HPBarSystem.lastHP = -1
        HPBarSystem.lastMax = -1
        return
    end

    ensureUI()

    local bossId, bossHp, bossMaxHp = getBossLife()
    if bossId and bossHp and bossMaxHp then
        ensureBossUI()
        bossHp = math.max(0, bossHp)
        bossMaxHp = math.max(1, bossMaxHp)

        local screenW = _G.SCREEN_WIDTH or 800
        local screenH = _G.SCREEN_HEIGHT or 600
        local bossW = math.min(HPBarSystem.bossMaxWidth, screenW - 120)
        local bossX = (screenW - bossW) / 2
        local bossY = screenH - 42
        local bossRatio = math.max(0, math.min(1, bossHp / bossMaxHp))
        local bossFillW = math.max(2, (bossW - 4) * bossRatio)

        if bossId ~= HPBarSystem.lastBossId or bossHp ~= HPBarSystem.lastBossHP or bossMaxHp ~= HPBarSystem.lastBossMax then
            HPBarSystem.lastBossId = bossId
            HPBarSystem.lastBossHP = bossHp
            HPBarSystem.lastBossMax = bossMaxHp
            ECS.setRect(HPBarSystem.bossBgId, bossX, bossY, bossW, HPBarSystem.bossHeight)
            ECS.setRect(HPBarSystem.bossFillId, bossX + 2, bossY + 2, bossFillW, HPBarSystem.bossHeight - 4)
            ECS.setUIColor(HPBarSystem.bossFillId, 0.9, 0.1, 0.08)
            ECS.setUIPosition(HPBarSystem.bossTextId, bossX + bossW / 2 - 56, bossY + 2)
            ECS.setUIText(HPBarSystem.bossTextId, "BOSS: " .. tostring(math.floor(bossHp)) .. "/" .. tostring(bossMaxHp))
        end
    else
        hideBossUI()
    end

    local hp, maxHp = getLocalPlayerLife()
    if hp == nil or maxHp == nil then
        return
    end

    hp = math.max(0, hp)
    maxHp = math.max(1, maxHp)

    if hp == HPBarSystem.lastHP and maxHp == HPBarSystem.lastMax then
        return
    end

    HPBarSystem.lastHP = hp
    HPBarSystem.lastMax = maxHp

    local ratio = math.max(0, math.min(1, hp / maxHp))
    local fillW = math.max(2, (HPBarSystem.maxWidth - 4) * ratio)
    local r, g, b = hpColor(ratio)

    ECS.setRect(HPBarSystem.fillId, 26, 66, fillW, HPBarSystem.height - 4)
    ECS.setUIColor(HPBarSystem.fillId, r, g, b)
    ECS.setUIText(HPBarSystem.textId, "HP: " .. tostring(math.floor(hp)) .. "/" .. tostring(maxHp))
end

ECS.registerSystem(HPBarSystem)
return HPBarSystem
