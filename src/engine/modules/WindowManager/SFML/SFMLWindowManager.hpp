#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include "../IWindowManager.hpp"

namespace rtypeEngine {
class SFMLWindowManager : public IWindowManager {
  public:
    explicit SFMLWindowManager(const char* pubEndpoint, const char* subEndpoint);
    ~SFMLWindowManager() override = default;

    void init() override;
    void loop() override;  // DEPRECATED: Use fixedUpdate() via hard-wired calls
    void cleanup() override;

    // ═══════════════════════════════════════════════════════════════
    // HARD-WIRED GAME LOOP (High Performance)
    // ═══════════════════════════════════════════════════════════════
    void fixedUpdate(double /*dt*/) override;
    void render(double /*alpha*/) override;

    void createWindow(const std::string &title, const Vector2u &size) override;
    bool isOpen() const override;
    void close() override;
    void drawPixels(const std::vector<uint32_t> &pixels, const Vector2u &size) override;

  private:
    void handleImageRendered(const std::string& message);
    void handleSetFullscreen(const std::string& message);
    void handleSetWindowSize(const std::string& message);
    void handleGetWindowInfo(const std::string& message);
    void recreateWindow(bool fullscreen);
    
    std::unique_ptr<sf::RenderWindow> _window;
    sf::Texture _texture;
    sf::Sprite _sprite;
    
    std::string _windowTitle = "R-Type Clone";
    Vector2u _windowedSize = {800, 600};
    bool _isFullscreen = false;
};
}  // namespace rtypeEngine
