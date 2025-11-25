
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <utility>

#include "../modules/IModule.hpp"
#include "../modulesManager/AModulesManager.hpp"

namespace rtypeEngine {
class IApplication {
  public:
    using MessageHandler = std::function<void(const std::string&)>;
    using TopicSubscription = std::pair<std::string, MessageHandler>;

    virtual ~IApplication() = default;
    virtual void addModule(const std::string &modulePath, const std::string &pubEndpoint, const std::string &subEndpoint) = 0;
    virtual void run() = 0;
    virtual void init() = 0;
    virtual void loop() = 0;

    virtual void sendMessage(const std::string& topic, const std::string& message) = 0;
    virtual std::string getMessage(const std::string& topic) = 0;
    virtual void subscribe(const std::string& topic, MessageHandler handler) = 0;
    virtual void unsubscribe(const std::string& topic) = 0;
    virtual void processMessages() = 0;

  protected:
    std::shared_ptr<AModulesManager> _modulesManager;
    std::vector<std::shared_ptr<IModule>> _modules;
    std::vector<TopicSubscription> _subscriptions;
    std::string _endpoint;
    bool _running = false;
};
}  // namespace rtypeEngine
