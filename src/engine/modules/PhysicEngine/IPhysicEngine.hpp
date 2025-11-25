#pragma once

#include "../AModule.hpp"

namespace rtypeEngine {
class IPhysicEngine : public AModule {
  public:
    explicit IPhysicEngine(const char* pubEndpoint, const char* subEndpoint)
        : AModule(pubEndpoint, subEndpoint) {}
    virtual ~IPhysicEngine() = default;

  protected:
    virtual void createBody(const std::string& id, const std::string& type, const std::vector<float>& params) = 0;
    virtual void setTransform(const std::string& id, const std::vector<float>& pos, const std::vector<float>& rot) = 0;
    virtual void applyForce(const std::string& id, const std::vector<float>& force) = 0;
};
}  // namespace rtypeEngine
