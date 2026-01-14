-- ═══════════════════════════════════════════════════════════════════════════════
-- ECS Compatibility Wrapper (v1.0)
--
-- Purpose: Ensure code written by different team members is compatible
-- Provides both camelCase (current) and snake_case aliases
-- Acts as bridge for physics bindings and network features
--
-- Load at TOP of any gameplay script: require("ecs_compat")
-- ═══════════════════════════════════════════════════════════════════════════════

-- Ensure ECS table exists
if not ECS then
    error("[ecs_compat] FATAL: ECS API not initialized. Ensure GameEngine calls init()")
end

-- Store original ECS table
local _ECS_ORIGINAL = ECS

-- Create new ECS with both naming conventions
ECS = {}

-- ═══════════════════════════════════════════════════════════════════════════════
-- ENTITY MANAGEMENT
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.createEntity = _ECS_ORIGINAL.createEntity
ECS.create_entity = _ECS_ORIGINAL.createEntity

ECS.destroyEntity = _ECS_ORIGINAL.destroyEntity
ECS.destroy_entity = _ECS_ORIGINAL.destroyEntity

-- ═══════════════════════════════════════════════════════════════════════════════
-- COMPONENT MANAGEMENT
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.addComponent = _ECS_ORIGINAL.addComponent
ECS.add_component = _ECS_ORIGINAL.addComponent

ECS.getComponent = _ECS_ORIGINAL.getComponent
ECS.get_component = _ECS_ORIGINAL.getComponent

ECS.removeComponent = _ECS_ORIGINAL.removeComponent
ECS.remove_component = _ECS_ORIGINAL.removeComponent

ECS.hasComponent = _ECS_ORIGINAL.hasComponent
ECS.has_component = _ECS_ORIGINAL.hasComponent

ECS.updateComponent = _ECS_ORIGINAL.updateComponent
ECS.update_component = _ECS_ORIGINAL.updateComponent

ECS.getEntitiesWith = _ECS_ORIGINAL.getEntitiesWith
ECS.get_entities_with = _ECS_ORIGINAL.getEntitiesWith

-- ═══════════════════════════════════════════════════════════════════════════════
-- RENDERING
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.createMesh = _ECS_ORIGINAL.createMesh
ECS.create_mesh = _ECS_ORIGINAL.createMesh

ECS.createText = _ECS_ORIGINAL.createText
ECS.create_text = _ECS_ORIGINAL.createText

ECS.setText = _ECS_ORIGINAL.setText
ECS.set_text = _ECS_ORIGINAL.setText

ECS.setTexture = _ECS_ORIGINAL.setTexture
ECS.set_texture = _ECS_ORIGINAL.setTexture

ECS.syncToRenderer = _ECS_ORIGINAL.syncToRenderer
ECS.sync_to_renderer = _ECS_ORIGINAL.syncToRenderer

-- ═══════════════════════════════════════════════════════════════════════════════
-- PHYSICS & COLLIDERS (NEW - Critical for multiplayer)
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.setCollider = _ECS_ORIGINAL.setCollider
ECS.set_collider = _ECS_ORIGINAL.setCollider

ECS.getCollider = _ECS_ORIGINAL.getCollider
ECS.get_collider = _ECS_ORIGINAL.getCollider

ECS.removeCollider = _ECS_ORIGINAL.removeCollider
ECS.remove_collider = _ECS_ORIGINAL.removeCollider

-- ═══════════════════════════════════════════════════════════════════════════════
-- AUDIO
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.playSound = _ECS_ORIGINAL.playSound
ECS.play_sound = _ECS_ORIGINAL.playSound

-- Music bindings (if available)
if _ECS_ORIGINAL.playMusic then
    ECS.playMusic = _ECS_ORIGINAL.playMusic
    ECS.play_music = _ECS_ORIGINAL.playMusic

    ECS.stopMusic = _ECS_ORIGINAL.stopMusic
    ECS.stop_music = _ECS_ORIGINAL.stopMusic

    ECS.pauseMusic = _ECS_ORIGINAL.pauseMusic
    ECS.pause_music = _ECS_ORIGINAL.pauseMusic

    ECS.resumeMusic = _ECS_ORIGINAL.resumeMusic
    ECS.resume_music = _ECS_ORIGINAL.resumeMusic

    ECS.setMusicVolume = _ECS_ORIGINAL.setMusicVolume
    ECS.set_music_volume = _ECS_ORIGINAL.setMusicVolume
end

-- ═══════════════════════════════════════════════════════════════════════════════
-- INPUT
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.isKeyPressed = _ECS_ORIGINAL.isKeyPressed
ECS.is_key_pressed = _ECS_ORIGINAL.isKeyPressed

-- ═══════════════════════════════════════════════════════════════════════════════
-- MESSAGING & NETWORKING
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.sendMessage = _ECS_ORIGINAL.sendMessage
ECS.send_message = _ECS_ORIGINAL.sendMessage

ECS.subscribe = _ECS_ORIGINAL.subscribe

ECS.sendNetworkMessage = _ECS_ORIGINAL.sendNetworkMessage
ECS.send_network_message = _ECS_ORIGINAL.sendNetworkMessage

ECS.broadcastNetworkMessage = _ECS_ORIGINAL.broadcastNetworkMessage
ECS.broadcast_network_message = _ECS_ORIGINAL.broadcastNetworkMessage

ECS.sendToClient = _ECS_ORIGINAL.sendToClient
ECS.send_to_client = _ECS_ORIGINAL.sendToClient

ECS.sendBinary = _ECS_ORIGINAL.sendBinary
ECS.send_binary = _ECS_ORIGINAL.sendBinary

ECS.broadcastBinary = _ECS_ORIGINAL.broadcastBinary
ECS.broadcast_binary = _ECS_ORIGINAL.broadcastBinary

ECS.sendToClientBinary = _ECS_ORIGINAL.sendToClientBinary
ECS.send_to_client_binary = _ECS_ORIGINAL.sendToClientBinary

ECS.unpackMsgPack = _ECS_ORIGINAL.unpackMsgPack
ECS.unpack_msgpack = _ECS_ORIGINAL.unpackMsgPack

-- ═══════════════════════════════════════════════════════════════════════════════
-- GAME MODE & CAPABILITIES
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.setGameMode = _ECS_ORIGINAL.setGameMode
ECS.set_game_mode = _ECS_ORIGINAL.setGameMode

ECS.isServer = _ECS_ORIGINAL.isServer
ECS.is_server = _ECS_ORIGINAL.isServer

ECS.isLocalMode = _ECS_ORIGINAL.isLocalMode
ECS.is_local_mode = _ECS_ORIGINAL.isLocalMode

ECS.isClientMode = _ECS_ORIGINAL.isClientMode
ECS.is_client_mode = _ECS_ORIGINAL.isClientMode

-- Capabilities table (read-only flags)
ECS.capabilities = _ECS_ORIGINAL.capabilities

-- ═══════════════════════════════════════════════════════════════════════════════
-- SYSTEM & STATE MANAGEMENT
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.registerSystem = _ECS_ORIGINAL.registerSystem
ECS.register_system = _ECS_ORIGINAL.registerSystem

ECS.removeSystems = _ECS_ORIGINAL.removeSystems
ECS.remove_systems = _ECS_ORIGINAL.removeSystems

ECS.removeEntities = _ECS_ORIGINAL.removeEntities
ECS.remove_entities = _ECS_ORIGINAL.removeEntities

ECS.saveState = _ECS_ORIGINAL.saveState
ECS.save_state = _ECS_ORIGINAL.saveState

ECS.loadLastSave = _ECS_ORIGINAL.loadLastSave
ECS.load_last_save = _ECS_ORIGINAL.loadLastSave

ECS.loadFirstSave = _ECS_ORIGINAL.loadFirstSave
ECS.load_first_save = _ECS_ORIGINAL.loadFirstSave

ECS.getSaves = _ECS_ORIGINAL.getSaves
ECS.get_saves = _ECS_ORIGINAL.getSaves

-- ═══════════════════════════════════════════════════════════════════════════════
-- SCENE MANAGEMENT (TODO: Implement in engine)
-- ═══════════════════════════════════════════════════════════════════════════════
-- ECS.loadScene = function(sceneName)
--     -- Will implement after basic systems are stable
--     ECS.log("⚠️ Scene loading not yet implemented")
-- end

-- ═══════════════════════════════════════════════════════════════════════════════
-- LOGGING
-- ═══════════════════════════════════════════════════════════════════════════════
ECS.log = _ECS_ORIGINAL.log

-- ═══════════════════════════════════════════════════════════════════════════════
-- SAFETY CHECKS
-- ═══════════════════════════════════════════════════════════════════════════════

-- Verify critical functions exist
local required_functions = {
    "createEntity", "destroyEntity", "addComponent", "getComponent",
    "setCollider", "playSound", "isKeyPressed", "syncToRenderer"
}

for _, func_name in ipairs(required_functions) do
    if not ECS[func_name] then
        error("[ecs_compat] CRITICAL: Missing function ECS." .. func_name ..
              "\nCheck GameEngine initialization and LuaECSManager bindings")
    end
end

-- Log initialization success
if ECS.log then
    ECS.log("✅ ECS Compatibility Layer v1.0 loaded (camelCase + snake_case aliases)")
end

-- Return wrapper for require() compatibility
return ECS
