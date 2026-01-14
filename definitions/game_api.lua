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
--- @function createMesh
--- @param id string
--- @param path string
function ecs.createMesh(id, path) end

---
--- @function isKeyPressed
--- @param keyName string
function ecs.isKeyPressed(keyName) end

---
--- @function addComponent
--- @param entityId string
--- @param componentName string
--- @param componentData table
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

