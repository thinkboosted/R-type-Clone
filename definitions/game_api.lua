-- Auto-generated Lua API definitions
-- Generated from LuaECSManager.cpp

---
--- @function setGameMode
--- @param mode_name string
function ecs.setGameMode(mode_name) end

---
--- @function isServer
function ecs.isServer() end

---
--- @function isLocalMode
function ecs.isLocalMode() end

---
--- @function isClientMode
function ecs.isClientMode() end

---
--- @function log
--- @param message string
function ecs.log(message) end

---
--- @function createEntity
function ecs.createEntity() end

---
--- @function destroyEntity
--- @param id string
function ecs.destroyEntity(id) end

---
--- @function createText
--- @param id string
--- @param text string
--- @param fontPath string
--- @param fontSize integer
--- @param isScreenSpace boolean
function ecs.createText(id, text, fontPath, fontSize, isScreenSpace) end

---
--- @function setText
--- @param id string
--- @param text string
function ecs.setText(id, text) end

---
--- @function setTexture
--- @param id string
--- @param path string
function ecs.setTexture(id, path) end

---
--- @function playSound
--- @param path string
function ecs.playSound(path) end

---
--- @function playMusic
--- @param musicId string
--- @param path string
--- @param volume number
--- @param loop boolean
function ecs.playMusic(musicId, path, volume, loop) end

---
--- @function stopMusic
--- @param musicId string
function ecs.stopMusic(musicId) end

---
--- @function pauseMusic
--- @param musicId string
function ecs.pauseMusic(musicId) end

---
--- @function resumeMusic
--- @param musicId string
function ecs.resumeMusic(musicId) end

---
--- @function setMusicVolume
--- @param musicId string
--- @param volume number
function ecs.setMusicVolume(musicId, volume) end

---
--- @function createMesh
--- @param id string
--- @param path string
function ecs.createMesh(id, path) end

---
--- @function setCollider
--- @param id string
--- @param type string
--- @param sx number
--- @param sy number
--- @param sz number
function ecs.setCollider(id, type, sx, sy, sz) end

---
--- @function getCollider
--- @param id string
function ecs.getCollider(id) end

---
--- @function removeCollider
--- @param id string
function ecs.removeCollider(id) end

---
--- @function setEntityOwner
--- @param id string
--- @param clientId integer
function ecs.setEntityOwner(id, clientId) end

---
--- @function getEntityOwner
--- @param id string
function ecs.getEntityOwner(id) end

---
--- @function isEntityOwned
--- @param id string
function ecs.isEntityOwned(id) end

---
--- @function canModifyEntity
--- @param id string
function ecs.canModifyEntity(id) end

---
--- @function setClientId
--- @param clientId integer
function ecs.setClientId(clientId) end

---
--- @function getClientId
function ecs.getClientId() end

---
--- @function isKeyPressed
--- @param keyName string
function ecs.isKeyPressed(keyName) end

---
--- @function addComponent
--- @param entityId string
--- @param componentName string
--- @param componentData any
function ecs.addComponent(entityId, componentName, componentData) end

---
--- @function updateComponent
--- @param id string
--- @param component string
--- @param data table
function ecs.updateComponent(id, component, data) end

---
--- @function syncToRenderer
function ecs.syncToRenderer() end

---
--- @function removeComponent
--- @param id string
--- @param name string
function ecs.removeComponent(id, name) end

---
--- @function hasComponent
--- @param id string
--- @param name string
function ecs.hasComponent(id, name) end

---
--- @function sendMessage
--- @param topic string
--- @param message string
function ecs.sendMessage(topic, message) end

---
--- @function subscribe
--- @param topic string
--- @param callback function
function ecs.subscribe(topic, callback) end

---
--- @function sendNetworkMessage
--- @param topic string
--- @param payload string
function ecs.sendNetworkMessage(topic, payload) end

---
--- @function broadcastNetworkMessage
--- @param topic string
--- @param payload string
function ecs.broadcastNetworkMessage(topic, payload) end

---
--- @function sendToClient
--- @param clientId integer
--- @param topic string
--- @param payload string
function ecs.sendToClient(clientId, topic, payload) end

---
--- @function syncEntityState
--- @param entityId string
--- @param x number
--- @param y number
--- @param z number
function ecs.syncEntityState(entityId, x, y, z) end

---
--- @function broadcastPhysicsUpdate
--- @param message string
function ecs.broadcastPhysicsUpdate(message) end

---
--- @function sendBinary
--- @param topic string
--- @param data any
function ecs.sendBinary(topic, data) end

---
--- @function broadcastBinary
--- @param topic string
--- @param data any
function ecs.broadcastBinary(topic, data) end

---
--- @function sendToClientBinary
--- @param clientId integer
--- @param topic string
--- @param data any
function ecs.sendToClientBinary(clientId, topic, data) end

---
--- @function unpackMsgPack
--- @param data string
function ecs.unpackMsgPack(data) end

---
--- @function splitClientIdAndMessage
--- @param data string
function ecs.splitClientIdAndMessage(data) end

---
--- @function registerSystem
--- @param system table
function ecs.registerSystem(system) end

---
--- @function saveState
--- @param saveName string
function ecs.saveState(saveName) end

---
--- @function loadLastSave
--- @param saveName string
function ecs.loadLastSave(saveName) end

---
--- @function loadFirstSave
--- @param saveName string
function ecs.loadFirstSave(saveName) end

---
--- @function getSaves
--- @param saveName string
function ecs.getSaves(saveName) end

---
--- @function removeSystems
function ecs.removeSystems() end

---
--- @function removeEntities
function ecs.removeEntities() end

---
--- @function loadScene
--- @param sceneName string
function ecs.loadScene(sceneName) end

---
--- @function getCurrentScene
function ecs.getCurrentScene() end

