local WinSystem = {}

function WinSystem.init()
    print("[WinSystem] Initialized")
end

function WinSystem.update(dt)
end

function WinSystem.onCollision(id1, id2)
    local isPlayer1 = ECS.getComponent(id1, "Player")
    local isWinCube1 = ECS.getComponent(id1, "WinCube")

    local isPlayer2 = ECS.getComponent(id2, "Player")
    local isWinCube2 = ECS.getComponent(id2, "WinCube")

    if (isPlayer1 and isWinCube2) then
        print("You won!")
        ECS.destroyEntity(id2)
    elseif (isPlayer2 and isWinCube1) then
        print("You won!")
        ECS.destroyEntity(id1)
    end
end

ECS.registerSystem(WinSystem)
