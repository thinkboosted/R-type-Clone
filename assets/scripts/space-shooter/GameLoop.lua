-- ────────────────────────────────────────────────────────────────────────────
-- Core Gameplay Systems (Run on Authority: Server + Solo)
-- ────────────────────────────────────────────────────────────────────────────
print("[GameLoop] Loading Core Gameplay Systems...")

dofile("assets/scripts/space-shooter/systems/CollisionSystem.lua")
dofile("assets/scripts/space-shooter/systems/PhysicSystem.lua")
dofile("assets/scripts/space-shooter/systems/LifeSystem.lua")
dofile("assets/scripts/space-shooter/systems/WinSystem.lua")
dofile("assets/scripts/space-shooter/systems/EnemySystem.lua")
dofile("assets/scripts/space-shooter/systems/PlayerSystem.lua")
dofile("assets/scripts/space-shooter/systems/GameStateManager.lua")

-- ────────────────────────────────────────────────────────────────────────────
-- Input & Control Systems (Run everywhere, adapt based on mode)
-- ────────────────────────────────────────────────────────────────────────────
print("[GameLoop] Loading Input System...")
dofile("assets/scripts/space-shooter/systems/InputSystem.lua")

-- ────────────────────────────────────────────────────────────────────────────
-- Network Synchronization (Run on Server + Client in Multiplayer)
-- ────────────────────────────────────────────────────────────────────────────
if ECS.capabilities.hasNetworkSync then
    print("[GameLoop] Loading Network System...")
    dofile("assets/scripts/space-shooter/systems/NetworkSystem.lua")
end

-- ────────────────────────────────────────────────────────────────────────────
-- Menu & Game State Management (Run on ALL instances for state tracking)
-- ────────────────────────────────────────────────────────────────────────────
print("[GameLoop] Loading Menu & Game State System...")
dofile("assets/scripts/space-shooter/systems/MenuSystem.lua")

-- ────────────────────────────────────────────────────────────────────────────
-- Visual & UI Systems (Run on Client + Solo)
-- ────────────────────────────────────────────────────────────────────────────
if ECS.capabilities.hasRendering then
    print("[GameLoop] Loading Visual & UI Systems...")
    dofile("assets/scripts/space-shooter/systems/RenderSystem.lua")
    dofile("assets/scripts/space-shooter/systems/AnimationSystem.lua")
    dofile("assets/scripts/space-shooter/systems/ParticleSystem.lua")
    dofile("assets/scripts/space-shooter/systems/ScoreSystem.lua")
    dofile("assets/scripts/space-shooter/systems/BackgroundSystem.lua")
    dofile("assets/scripts/space-shooter/systems/HitFlashSystem.lua")

    -- NOTE: Camera is now created by MenuSystem when menu is rendered,
    -- and by the game when transitioning to gameplay.
    -- We don't create a default camera here to avoid conflicts.
    print("[GameLoop] Visual systems loaded (camera managed by MenuSystem)")
end

-- ============================================================================
-- GAME STATE INITIALIZATION
-- ============================================================================

-- All instances start with game NOT running, waiting for menu transition to PLAYING
ECS.isGameRunning = false

if ECS.capabilities.hasAuthority then
    if ECS.isLocalMode() then
        print("[GameLoop] Solo Mode initialized - awaiting menu interaction")
    else
        print("[GameLoop] Server Mode initialized - awaiting START command or client input")
    end
end