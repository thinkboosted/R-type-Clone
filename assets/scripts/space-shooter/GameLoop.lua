-- ============================================================================
-- GameLoop.lua - Unified Game Loop for Solo & Multiplayer
-- ============================================================================
-- This file replaces GameClient.lua and GameServer.lua to provide a single
-- source of truth for game logic. All Systems are loaded here and adapt their
-- behavior based on ECS.capabilities flags instead of checking ECS.isServer()
-- directly throughout the codebase.
--
-- Architecture:
-- - Solo Mode: Client has Authority + Rendering (Server-like behavior)
-- - Server Mode: Has Authority only (no rendering)
-- - Client Mode: Has Rendering only (receives state from server)
-- ============================================================================

print("=== Loading Unified GameLoop ===")

-- Load Component Definitions first
dofile("assets/scripts/space-shooter/components/Components.lua")

-- ============================================================================
-- CAPABILITY FLAGS SYSTEM
-- ============================================================================
-- Capabilities are now managed by the C++ engine and exposed via ECS.capabilities.
-- We do NOT overwrite them here to maintain the reference.

-- Debug: Print current capabilities
if ECS.capabilities then
    print("[GameLoop] Capabilities (from C++):")
    print("  - Authority (Simulation): " .. tostring(ECS.capabilities.hasAuthority))
    print("  - Rendering (Visuals):    " .. tostring(ECS.capabilities.hasRendering))
    print("  - Network Sync:           " .. tostring(ECS.capabilities.hasNetworkSync))
    print("  - Local Input:            " .. tostring(ECS.capabilities.hasLocalInput))
else
    print("[GameLoop] ERROR: ECS.capabilities is nil!")
end

-- ============================================================================
-- SYSTEM LOADING ORDER
-- ============================================================================
-- Systems are loaded in dependency order. Each system auto-configures based
-- on ECS.capabilities. No more separate client/server bootstraps!

-- ────────────────────────────────────────────────────────────────────────────
-- Core Gameplay Systems (Run on Authority: Server + Solo)
-- ────────────────────────────────────────────────────────────────────────────
print("[GameLoop] Loading Core Gameplay Systems...")

dofile("assets/scripts/space-shooter/systems/CollisionSystem.lua")
dofile("assets/scripts/space-shooter/systems/PhysicSystem.lua")
dofile("assets/scripts/space-shooter/systems/LifeSystem.lua")
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
    dofile("assets/scripts/space-shooter/systems/ParticleSystem.lua")
    dofile("assets/scripts/space-shooter/systems/ScoreSystem.lua")

    -- Setup Camera (only needed for rendering instances)
    local camera = ECS.createEntity()
    ECS.addComponent(camera, "Transform", Transform(0, 0, 20))
    ECS.addComponent(camera, "Camera", Camera(90))
    print("[GameLoop] Camera created")
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
else
    print("[GameLoop] Client Mode initialized - awaiting server assignment")
end

print("=== GameLoop Load Complete ===")