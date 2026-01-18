/**
 * @file SFMLWindowManager.hpp
 * @brief SFML-based window management module
 * 
 * @details Manages the application window and handles input events using SFML.
 * Displays rendered frames and forwards input events to the ECS.
 * 
 * @section channels_sub Subscribed Channels
 * | Channel | Payload | Description |
 * |---------|---------|-------------|
 * | `ImageRendered` | Raw pixel data | Display rendered frame from Renderer |
 * | `SetFullscreen` | "true"/"false" | Toggle fullscreen mode |
 * | `SetWindowSize` | "width,height" | Resize window |
 * | `GetWindowInfo` | - | Request window info |
 * 
 * @section channels_pub Published Channels
 * | Channel | Payload | Description |
 * |---------|---------|-------------|
 * | `KeyPressed` | Key name | Key press event (UP, DOWN, SPACE, etc.) |
 * | `KeyReleased` | Key name | Key release event |
 * | `MousePressed` | "x,y,button" | Mouse click event |
 * | `MouseMoved` | "x,y" | Mouse movement event |
 * | `WindowResized` | "width,height" | Window resize event |
 * | `WindowInfo` | "width,height,fullscreen" | Response to GetWindowInfo |
 * 
 * @see docs/CHANNELS.md for complete channel reference
 */

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
    void loop() override;
    void cleanup() override;

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
