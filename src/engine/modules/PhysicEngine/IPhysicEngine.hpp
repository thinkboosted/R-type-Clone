/**
 * @file IPhysicEngine.hpp
 * @brief Interface for physics engine modules
 * 
 * @details Defines the contract for physics simulation implementations.
 * 
 * @see BulletPhysicEngine for Bullet3 implementation
 * @see docs/CHANNELS.md for channel reference
 */

#pragma once

#include "../AModule.hpp"

namespace rtypeEngine {
class IPhysicEngine : public AModule {
  public:
    using AModule::AModule;
    virtual ~IPhysicEngine() = default;

  protected:
    virtual void createBody(const std::string& id, const std::string& type, const std::vector<float>& params) = 0;
    virtual void setTransform(const std::string& id, const std::vector<float>& pos, const std::vector<float>& rot) = 0;
    virtual void applyForce(const std::string& id, const std::vector<float>& force) = 0;
};
}  // namespace rtypeEngine
