#pragma once

#include "../AModule.hpp"
#include "../../types/vector.hpp"
#include <string>
#include <vector>

namespace rtypeEngine {

    using EntityID = std::string;

    enum class EntityType {
        EMPTY,
        MESH,
        TEXT,
        UI_COMPONENT,
        CAMERA,
        LIGHT
    };

    struct RenderCommand {
        EntityID entityId;
        std::string meshPath;
        std::string texturePath;
        Vector3f position;
        Vector3f rotation;
        Vector3f scale;
        bool isVisible;
    };

    class I3DSceneManager : public AModule {
    public:
        explicit I3DSceneManager(const char* pubEndpoint, const char* subEndpoint)
            : AModule(pubEndpoint, subEndpoint) {}
        virtual ~I3DSceneManager() = default;

        // Entity Lifecycle
        virtual EntityID createEntity(EntityType type) = 0;
        virtual void deleteEntity(const EntityID& id) = 0;

        // Transform Management
        virtual void setPosition(const EntityID& id, const Vector3f& position) = 0;
        virtual void move(const EntityID& id, const Vector3f& offset) = 0;
        virtual Vector3f getPosition(const EntityID& id) const = 0;

        virtual void setRotation(const EntityID& id, const Vector3f& rotation) = 0;
        virtual void rotate(const EntityID& id, const Vector3f& delta) = 0;
        virtual Vector3f getRotation(const EntityID& id) const = 0;

        virtual void setScale(const EntityID& id, const Vector3f& scale) = 0;
        virtual Vector3f getScale(const EntityID& id) const = 0;

        // Component Assignment
        virtual void setMesh(const EntityID& id, const std::string& meshPath) = 0;
        virtual void setTexture(const EntityID& id, const std::string& texturePath) = 0;

        // Collider: assuming box collider for simplicity as a start
        virtual void setCollider(const EntityID& id, const Vector3f& size, const Vector3f& offset = {0, 0, 0}) = 0;

        // Text/UI
        virtual void setText(const EntityID& id, const std::string& text, unsigned int fontSize = 24) = 0;

        // Camera Management
        virtual void setActiveCamera(const EntityID& id) = 0;
        virtual EntityID getActiveCamera() const = 0;

        // Light Management
        virtual void setLightProperties(const EntityID& id, const Vector3f& color, float intensity) = 0;

        // Rendering
        // Returns a list of render commands for objects visible to the active camera
        virtual std::vector<RenderCommand> getRenderQueue() = 0;
    };

} // namespace rtypeEngine
