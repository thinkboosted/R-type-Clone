#pragma once

#include "../I3DSceneManager.hpp"
#include "../../AModule.hpp"
#include <unordered_map>
#include <string>
#include <memory>

namespace rtypeEngine {

    struct Entity {
        EntityID id;
        EntityType type;
        Vector3f position = {0.0f, 0.0f, 0.0f};
        Vector3f rotation = {0.0f, 0.0f, 0.0f};
        Vector3f scale = {1.0f, 1.0f, 1.0f};

        std::string meshPath;
        std::string texturePath;

        // Collider
        bool hasCollider = false;
        Vector3f colliderSize = {0.0f, 0.0f, 0.0f};
        Vector3f colliderOffset = {0.0f, 0.0f, 0.0f};

        // Text
        std::string text;
        unsigned int fontSize = 24;

        // Light
        Vector3f lightColor = {1.0f, 1.0f, 1.0f};
        float lightIntensity = 1.0f;
    };

    class EngineSceneManager : public I3DSceneManager {
    public:
        EngineSceneManager(const char* pubEndpoint, const char* subEndpoint);
        ~EngineSceneManager() override;

        // IModule overrides
        void init() override;
        void loop() override;
        void cleanup() override;

        // I3DSceneManager overrides
        EntityID createEntity(EntityType type) override;
        void deleteEntity(const EntityID& id) override;

        void setPosition(const EntityID& id, const Vector3f& position) override;
        void move(const EntityID& id, const Vector3f& offset) override;
        Vector3f getPosition(const EntityID& id) const override;

        void setRotation(const EntityID& id, const Vector3f& rotation) override;
        void rotate(const EntityID& id, const Vector3f& delta) override;
        Vector3f getRotation(const EntityID& id) const override;

        void setScale(const EntityID& id, const Vector3f& scale) override;
        Vector3f getScale(const EntityID& id) const override;

        void setMesh(const EntityID& id, const std::string& meshPath) override;
        void setTexture(const EntityID& id, const std::string& texturePath) override;

        void setCollider(const EntityID& id, const Vector3f& size, const Vector3f& offset) override;

        void setText(const EntityID& id, const std::string& text, unsigned int fontSize) override;

        void setActiveCamera(const EntityID& id) override;
        EntityID getActiveCamera() const override;

        void setLightProperties(const EntityID& id, const Vector3f& color, float intensity) override;

        std::vector<RenderCommand> getRenderQueue() override;

    private:
        std::unordered_map<EntityID, std::shared_ptr<Entity>> _entities;
        EntityID _activeCameraId;
        EntityID _cubeId;
        EntityID _lightId;

        std::string generateUuid();
    };

} // namespace rtypeEngine
