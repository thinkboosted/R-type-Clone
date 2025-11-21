#include "EngineSceneManager.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <sstream>

namespace rtypeEngine {

    EngineSceneManager::EngineSceneManager(const char* pubEndpoint, const char* subEndpoint)
        : I3DSceneManager(pubEndpoint, subEndpoint) {
    }

    EngineSceneManager::~EngineSceneManager() {
    }

    void EngineSceneManager::init() {
        std::cout << "[EngineSceneManager] Initialized" << std::endl;

        subscribe("SceneCommand", [this](const std::string& msg) {
            this->onSceneCommand(msg);
        });

        // Create Camera
        _activeCameraId = createEntity(EntityType::CAMERA);
        setPosition(_activeCameraId, {0.0f, 0.0f, 5.0f});
        setActiveCamera(_activeCameraId);

    }

    void EngineSceneManager::loop() {
        // Serialize Scene
        std::stringstream ss;

        // Camera
        if (!_activeCameraId.empty()) {
            Vector3f camPos = getPosition(_activeCameraId);
            ss << "Camera:" << _activeCameraId << "," << camPos.x << "," << camPos.y << "," << camPos.z << ";";
        }

        // Meshes
        for (auto& pair : _entities) {
            if (pair.second->type == EntityType::MESH) {
                auto& e = pair.second;
                ss << "Mesh:" << e->id << "," << e->meshPath << ","
                   << e->position.x << "," << e->position.y << "," << e->position.z << ","
                   << e->rotation.x << "," << e->rotation.y << "," << e->rotation.z << ","
                   << e->scale.x << "," << e->scale.y << "," << e->scale.z << ";";
            } else if (pair.second->type == EntityType::LIGHT) {
                auto& e = pair.second;
                ss << "Light:" << e->id << ","
                   << e->position.x << "," << e->position.y << "," << e->position.z << ","
                   << e->lightColor.x << "," << e->lightColor.y << "," << e->lightColor.z << ","
                   << e->lightIntensity << ";";
            }
        }


        sendMessage("SceneUpdated", ss.str());
    }

    void EngineSceneManager::cleanup() {
        _entities.clear();
        std::cout << "[EngineSceneManager] Cleaned up" << std::endl;
    }

    EntityID EngineSceneManager::createEntity(EntityType type) {
        auto id = generateUuid();
        auto entity = std::make_shared<Entity>();
        entity->id = id;
        entity->type = type;
        _entities[id] = entity;
        return id;
    }

    void EngineSceneManager::deleteEntity(const EntityID& id) {
        _entities.erase(id);
    }

    void EngineSceneManager::setPosition(const EntityID& id, const Vector3f& position) {
        if (_entities.find(id) != _entities.end()) {
            _entities[id]->position = position;
        }
    }

    void EngineSceneManager::move(const EntityID& id, const Vector3f& offset) {
        if (_entities.find(id) != _entities.end()) {
            auto& pos = _entities[id]->position;
            pos.x += offset.x;
            pos.y += offset.y;
            pos.z += offset.z;
        }
    }

    Vector3f EngineSceneManager::getPosition(const EntityID& id) const {
        if (_entities.find(id) != _entities.end()) {
            return _entities.at(id)->position;
        }
        return {0, 0, 0};
    }

    void EngineSceneManager::setRotation(const EntityID& id, const Vector3f& rotation) {
        if (_entities.find(id) != _entities.end()) {
            _entities[id]->rotation = rotation;
        }
    }

    void EngineSceneManager::rotate(const EntityID& id, const Vector3f& delta) {
        if (_entities.find(id) != _entities.end()) {
            auto& rot = _entities[id]->rotation;
            rot.x += delta.x;
            rot.y += delta.y;
            rot.z += delta.z;
        }
    }

    Vector3f EngineSceneManager::getRotation(const EntityID& id) const {
        if (_entities.find(id) != _entities.end()) {
            return _entities.at(id)->rotation;
        }
        return {0, 0, 0};
    }

    void EngineSceneManager::setScale(const EntityID& id, const Vector3f& scale) {
        if (_entities.find(id) != _entities.end()) {
            _entities[id]->scale = scale;
        }
    }

    Vector3f EngineSceneManager::getScale(const EntityID& id) const {
        if (_entities.find(id) != _entities.end()) {
            return _entities.at(id)->scale;
        }
        return {1, 1, 1};
    }

    void EngineSceneManager::setMesh(const EntityID& id, const std::string& meshPath) {
        if (_entities.find(id) != _entities.end()) {
            _entities[id]->meshPath = meshPath;
        }
    }

    void EngineSceneManager::setTexture(const EntityID& id, const std::string& texturePath) {
        if (_entities.find(id) != _entities.end()) {
            _entities[id]->texturePath = texturePath;
        }
    }

    void EngineSceneManager::setCollider(const EntityID& id, const Vector3f& size, const Vector3f& offset) {
        if (_entities.find(id) != _entities.end()) {
            auto& entity = _entities[id];
            entity->hasCollider = true;
            entity->colliderSize = size;
            entity->colliderOffset = offset;
        }
    }

    void EngineSceneManager::setText(const EntityID& id, const std::string& text, unsigned int fontSize) {
        if (_entities.find(id) != _entities.end()) {
            _entities[id]->text = text;
            _entities[id]->fontSize = fontSize;
        }
    }

    void EngineSceneManager::setActiveCamera(const EntityID& id) {
        if (_entities.find(id) != _entities.end() && _entities[id]->type == EntityType::CAMERA) {
            _activeCameraId = id;
        }
    }

    EntityID EngineSceneManager::getActiveCamera() const {
        return _activeCameraId;
    }

    void EngineSceneManager::setLightProperties(const EntityID& id, const Vector3f& color, float intensity) {
        if (_entities.find(id) != _entities.end() && _entities[id]->type == EntityType::LIGHT) {
            _entities[id]->lightColor = color;
            _entities[id]->lightIntensity = intensity;
        }
    }

    std::vector<RenderCommand> EngineSceneManager::getRenderQueue() {
        std::vector<RenderCommand> queue;

        // Later, implement frustum culling here based on the active camera

        for (const auto& pair : _entities) {
            const auto& entity = pair.second;
            if (entity->type == EntityType::MESH || entity->type == EntityType::TEXT || entity->type == EntityType::UI_COMPONENT) {
                RenderCommand cmd;
                cmd.entityId = entity->id;
                cmd.meshPath = entity->meshPath;
                cmd.texturePath = entity->texturePath;
                cmd.position = entity->position;
                cmd.rotation = entity->rotation;
                cmd.scale = entity->scale;
                cmd.isVisible = true; // Assuming all are visible for now
                queue.push_back(cmd);
            }
        }
        return queue;
    }

    void EngineSceneManager::onSceneCommand(const std::string& message) {
        std::stringstream ss(message);
        std::string command;
        std::getline(ss, command, ':');

        if (command == "CreateEntity") {
            std::string typeStr;
            std::getline(ss, typeStr, ':');
            std::string id;
            std::getline(ss, id);

            EntityType type = EntityType::EMPTY;
            if (typeStr == "MESH") type = EntityType::MESH;
            else if (typeStr == "CAMERA") type = EntityType::CAMERA;
            else if (typeStr == "LIGHT") type = EntityType::LIGHT;

            if (!id.empty()) {
                auto entity = std::make_shared<Entity>();
                entity->id = id;
                entity->type = type;
                _entities[id] = entity;
                std::cout << "[EngineSceneManager] Created entity " << id << " of type " << typeStr << " (ID from Lua)" << std::endl;

                if (type == EntityType::MESH) {
                    setMesh(id, "assets/models/cube.obj");
                }
            } else {
                EntityID newId = createEntity(type);
                std::cout << "[EngineSceneManager] Created entity " << newId << " of type " << typeStr << std::endl;
                if (type == EntityType::MESH) {
                    setMesh(newId, "assets/models/cube.obj");
                }
            }
        } else if (command == "MoveEntity") {
            std::string params;
            std::getline(ss, params);
            std::stringstream paramSs(params);
            std::string id, xStr, yStr, zStr;
            std::getline(paramSs, id, ',');
            std::getline(paramSs, xStr, ',');
            std::getline(paramSs, yStr, ',');
            std::getline(paramSs, zStr, ',');

            try {
                float x = std::stof(xStr);
                float y = std::stof(yStr);
                float z = std::stof(zStr);
                move(id, {x, y, z});
                std::cout << "[EngineSceneManager] Moved entity " << id << " to " << x << "," << y << "," << z << std::endl;
            } catch (...) {
                std::cerr << "[EngineSceneManager] Invalid MoveEntity params: " << params << std::endl;
            }
        } else if (command == "RotateEntity") {
            std::string params;
            std::getline(ss, params);
            std::stringstream paramSs(params);
            std::string id, xStr, yStr, zStr;
            std::getline(paramSs, id, ',');
            std::getline(paramSs, xStr, ',');
            std::getline(paramSs, yStr, ',');
            std::getline(paramSs, zStr, ',');

            try {
                float x = std::stof(xStr);
                float y = std::stof(yStr);
                float z = std::stof(zStr);
                rotate(id, {x, y, z});
                std::cout << "[EngineSceneManager] Rotated entity " << id << " by " << x << "," << y << "," << z << std::endl;
            } catch (...) {
                std::cerr << "[EngineSceneManager] Invalid RotateEntity params: " << params << std::endl;
            }
        } else if (command == "SetPosition") {
            std::string params;
            std::getline(ss, params);
            std::stringstream paramSs(params);
            std::string id, xStr, yStr, zStr;
            std::getline(paramSs, id, ',');
            std::getline(paramSs, xStr, ',');
            std::getline(paramSs, yStr, ',');
            std::getline(paramSs, zStr, ',');

            try {
                float x = std::stof(xStr);
                float y = std::stof(yStr);
                float z = std::stof(zStr);
                setPosition(id, {x, y, z});
                std::cout << "[EngineSceneManager] Set position of entity " << id << " to " << x << "," << y << "," << z << std::endl;
            } catch (...) {
                std::cerr << "[EngineSceneManager] Invalid SetPosition params: " << params << std::endl;
            }
        } else if (command == "SetLightProperties") {
            std::string params;
            std::getline(ss, params);
            std::stringstream paramSs(params);
            std::string id, rStr, gStr, bStr, iStr;
            std::getline(paramSs, id, ',');
            std::getline(paramSs, rStr, ',');
            std::getline(paramSs, gStr, ',');
            std::getline(paramSs, bStr, ',');
            std::getline(paramSs, iStr, ',');

            try {
                float r = std::stof(rStr);
                float g = std::stof(gStr);
                float b = std::stof(bStr);
                float i = std::stof(iStr);
                setLightProperties(id, {r, g, b}, i);
                std::cout << "[EngineSceneManager] Set light properties for " << id << std::endl;
            } catch (...) {
                std::cerr << "[EngineSceneManager] Invalid SetLightProperties params: " << params << std::endl;
            }
        } else if (command == "SetActiveCamera") {
            std::string id;
            std::getline(ss, id);
            setActiveCamera(id);
            std::cout << "[EngineSceneManager] Set active camera to " << id << std::endl;
        }
    }

    std::string EngineSceneManager::generateUuid() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static std::uniform_int_distribution<> dis2(8, 11);

        std::stringstream ss;
        int i;
        ss << std::hex;
        for (i = 0; i < 8; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 4; i++) {
            ss << dis(gen);
        }
        ss << "-4";
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        ss << dis2(gen);
        for (i = 0; i < 3; i++) {
            ss << dis(gen);
        }
        ss << "-";
        for (i = 0; i < 12; i++) {
            ss << dis(gen);
        }
        return ss.str();
    }

} // namespace rtypeEngine

extern "C" rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::EngineSceneManager(pubEndpoint, subEndpoint);
}
