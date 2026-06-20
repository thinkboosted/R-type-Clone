local PlayerSystem = {}

function PlayerSystem.init()
    print("[PlayerSystem] Initialized")
end

function PlayerSystem.update(dt)
    if ECS.isPaused then return end

    -- PlayerSystem is intentionally lightweight; movement is handled in InputSystem,
    -- while firing is handled in WeaponSystem.
    if not ECS.capabilities.hasAuthority then return end
end

ECS.registerSystem(PlayerSystem)
return PlayerSystem
