#ifdef _WIN32
    #define SFML_WINDOW_MANAGER_EXPORT __declspec(dllexport)
#else
    #define SFML_WINDOW_MANAGER_EXPORT
#endif

#include "SFMLWindowManager.hpp"
#include "KeysMap.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <utility>
#include <map>
#include <sstream>

namespace rtypeEngine {

SFMLWindowManager::SFMLWindowManager(const char* pubEndpoint, const char* subEndpoint)
    : IWindowManager(pubEndpoint, subEndpoint), _window(nullptr), _texture(sf::Vector2u(1, 1)), _sprite(_texture),
      _windowTitle("R-Type Clone"), _windowedSize{800, 600}, _isFullscreen(false) {}

void SFMLWindowManager::init() {
    createWindow(_windowTitle, _windowedSize);
    subscribe("ImageRendered", [this](const std::string& message) {
        this->handleImageRendered(message);
    });
    subscribe("CloseWindow", [this](const std::string&) {
        this->close();
    });
    subscribe("SetFullscreen", [this](const std::string& message) {
        std::cout << "[SFMLWindowManager] Received SetFullscreen: " << message << std::endl;
        this->handleSetFullscreen(message);
    });
    subscribe("SetWindowSize", [this](const std::string& message) {
        std::cout << "[SFMLWindowManager] Received SetWindowSize: " << message << std::endl;
        this->handleSetWindowSize(message);
    });
    subscribe("ToggleFullscreen", [this](const std::string&) {
        std::cout << "[SFMLWindowManager] Received ToggleFullscreen" << std::endl;
        this->recreateWindow(!_isFullscreen);
    });
    subscribe("GetWindowInfo", [this](const std::string& message) {
        this->handleGetWindowInfo(message);
    });
    std::cout << "[SFMLWindowManager] Initialized" << std::endl;
}

void SFMLWindowManager::loop() {
    if (_window && _window->isOpen()) {
        while (auto event = _window->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window->close();
                sendMessage("ExitApplication", "");
            }
            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                std::stringstream ss;
                ss << static_cast<int>(mousePressed->button) << ":" << mousePressed->position.x << "," << mousePressed->position.y;
                sendMessage("MousePressed", ss.str());
            }
            if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                std::stringstream ss;
                ss << static_cast<int>(mouseReleased->button) << ":" << mouseReleased->position.x << "," << mouseReleased->position.y;
                sendMessage("MouseReleased", ss.str());
            }
            if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                std::stringstream ss;
                ss << mouseMoved->position.x << "," << mouseMoved->position.y;
                sendMessage("MouseMoved", ss.str());
            }
            if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
                auto it = keyMappings.find(keyReleased->code);
                if (it != keyMappings.end()) {
                    sendMessage("KeyReleased", it->second);
                }
            }
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                std::stringstream ss;
                ss << resized->size.x << "," << resized->size.y;
                sendMessage("WindowResized", ss.str());

                _texture = sf::Texture(sf::Vector2u(resized->size.x, resized->size.y));
                _sprite = sf::Sprite(_texture);
                _sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(static_cast<int>(resized->size.x), static_cast<int>(resized->size.y))));
                _sprite.setPosition(sf::Vector2f(0, 0));
                _sprite.setScale(sf::Vector2f(1.0f, 1.0f));

                sf::FloatRect visibleArea(sf::Vector2f(0, 0), sf::Vector2f(static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)));
                _window->setView(sf::View(visibleArea));
                
                // Update windowed size if not fullscreen
                if (!_isFullscreen) {
                    _windowedSize = {resized->size.x, resized->size.y};
                }
            }
        }
    }

    for (int k = 0; k < sf::Keyboard::KeyCount; ++k) {
        auto key = static_cast<sf::Keyboard::Key>(k);
        if (sf::Keyboard::isKeyPressed(key)) {
            sendMessage("KeyPressed", keyMappings.at(key));
        }
    }
}

void SFMLWindowManager::cleanup() {
    if (_window && _window->isOpen()) {
        _window->close();
    }
}

void SFMLWindowManager::createWindow(const std::string &title, const Vector2u &size) {
    _window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode(sf::Vector2u(size.x, size.y)), title);
    _texture = sf::Texture(sf::Vector2u(size.x, size.y));
    _sprite = sf::Sprite(_texture);
}

bool SFMLWindowManager::isOpen() const {
    return _window && _window->isOpen();
}

void SFMLWindowManager::close() {
    if (_window) {
        _window->close();
    }
}

void SFMLWindowManager::drawPixels(const std::vector<uint32_t> &pixels, const Vector2u &size) {
    if (!_window || !_window->isOpen()) {
        return;
    }

    _texture.update(reinterpret_cast<const std::uint8_t*>(pixels.data()));

    _window->clear();
    _window->draw(_sprite);
    _window->display();

}

void SFMLWindowManager::handleImageRendered(const std::string& pixelData) {
    // Message format: "<width>,<height>;<raw-pixel-bytes...>"
    // Parse header
    auto sep = pixelData.find(';');
    if (sep == std::string::npos) {
        std::cerr << "[SFMLWindowManager] handleImageRendered: missing header" << std::endl;
        return;
    }

    std::string header = pixelData.substr(0, sep);
    std::string body = pixelData.substr(sep + 1);

    unsigned int width = 0, height = 0;
    {
        std::stringstream ss(header);
        std::string wStr, hStr;
        if (!std::getline(ss, wStr, ',') || !std::getline(ss, hStr)) {
            std::cerr << "[SFMLWindowManager] handleImageRendered: invalid header='" << header << "'" << std::endl;
            return;
        }
        try {
            width = std::stoul(wStr);
            height = std::stoul(hStr);
        } catch (...) {
            std::cerr << "[SFMLWindowManager] handleImageRendered: error parsing header '" << header << "'" << std::endl;
            return;
        }
    }

    size_t pixelCount = body.size() / sizeof(uint32_t);
    if (pixelCount != static_cast<size_t>(width) * static_cast<size_t>(height)) {
        std::cerr << "[SFMLWindowManager] handleImageRendered: pixel size mismatch (got " << pixelCount << ", expected " << width * height << ")" << std::endl;
        return;
    }

    // If texture size doesn't match incoming image, recreate texture/sprite so update will succeed
    sf::Vector2u texSize = _texture.getSize();
    if (texSize.x != width || texSize.y != height) {
        std::cout << "[SFMLWindowManager] handleImageRendered: resizing texture from " << texSize.x << "x" << texSize.y << " to " << width << "x" << height << std::endl;
        _texture = sf::Texture(sf::Vector2u(width, height));
        _sprite = sf::Sprite(_texture);
        _sprite.setTextureRect(sf::IntRect(sf::Vector2i(0,0), sf::Vector2i(static_cast<int>(width), static_cast<int>(height))));
        _sprite.setPosition(sf::Vector2f(0,0));
        _sprite.setScale(sf::Vector2f(1.0f,1.0f));
    }

    const uint32_t* pixelPtr = reinterpret_cast<const uint32_t*>(body.data());
    std::vector<uint32_t> pixels(pixelPtr, pixelPtr + pixelCount);

    drawPixels(pixels, Vector2u{width, height});
}

void SFMLWindowManager::handleSetFullscreen(const std::string& message) {
    bool fullscreen = (message == "1" || message == "true");
    if (fullscreen != _isFullscreen) {
        recreateWindow(fullscreen);
    }
}

void SFMLWindowManager::handleSetWindowSize(const std::string& message) {
    std::stringstream ss(message);
    std::string widthStr, heightStr;
    if (std::getline(ss, widthStr, ',') && std::getline(ss, heightStr)) {
        try {
            unsigned int width = std::stoul(widthStr);
            unsigned int height = std::stoul(heightStr);
            
            std::cout << "[SFMLWindowManager] SetWindowSize requested: " << width << "x" << height << std::endl;
            
            if (width > 0 && height > 0) {
                _windowedSize = {width, height};
                
                // If in fullscreen, exit fullscreen first
                if (_isFullscreen) {
                    _isFullscreen = false;
                }
                
                // Recreate window with new size (SFML 3 requires this for reliable resizing)
                recreateWindow(false);
                
                std::cout << "[SFMLWindowManager] Window resized to " << width << "x" << height << std::endl;
            }
        } catch (...) {
            std::cerr << "[SFMLWindowManager] Error parsing window size" << std::endl;
        }
    }
}

void SFMLWindowManager::handleGetWindowInfo(const std::string& message) {
    if (_window) {
        sf::Vector2u size = _window->getSize();
        std::stringstream ss;
        ss << size.x << "," << size.y << "," << (_isFullscreen ? "1" : "0");
        sendMessage("WindowInfo", ss.str());
    }
}

void SFMLWindowManager::recreateWindow(bool fullscreen) {
    _isFullscreen = fullscreen;
    
    if (_window && _window->isOpen()) {
        _window->close();
    }
    
    if (fullscreen) {
        // Get desktop mode for fullscreen
        auto desktopMode = sf::VideoMode::getDesktopMode();
        _window = std::make_unique<sf::RenderWindow>(
            desktopMode, _windowTitle, sf::State::Fullscreen);
        
        sf::Vector2u size = _window->getSize();
        
        // Create texture and sprite with correct size
        _texture = sf::Texture(sf::Vector2u(size.x, size.y));
        _sprite = sf::Sprite(_texture);
        _sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(static_cast<int>(size.x), static_cast<int>(size.y))));
        _sprite.setPosition(sf::Vector2f(0, 0));
        _sprite.setScale(sf::Vector2f(1.0f, 1.0f));
        
        // Set the view to match the new size (1:1 pixel mapping)
        sf::FloatRect visibleArea(sf::Vector2f(0, 0), sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
        _window->setView(sf::View(visibleArea));
        
        // Notify renderer of size change
        std::stringstream resizeMsg;
        resizeMsg << size.x << "," << size.y;
        sendMessage("WindowResized", resizeMsg.str());
        
        std::cout << "[SFMLWindowManager] Switched to fullscreen: " << size.x << "x" << size.y << std::endl;
    } else {
        // Windowed mode
        _window = std::make_unique<sf::RenderWindow>(
            sf::VideoMode(sf::Vector2u(_windowedSize.x, _windowedSize.y)), _windowTitle, sf::Style::Default);
        
        // Create texture and sprite with correct size
        _texture = sf::Texture(sf::Vector2u(_windowedSize.x, _windowedSize.y));
        _sprite = sf::Sprite(_texture);
        _sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(static_cast<int>(_windowedSize.x), static_cast<int>(_windowedSize.y))));
        _sprite.setPosition(sf::Vector2f(0, 0));
        _sprite.setScale(sf::Vector2f(1.0f, 1.0f));
        
        // Set the view to match the new size (1:1 pixel mapping)
        sf::FloatRect visibleArea(sf::Vector2f(0, 0), sf::Vector2f(static_cast<float>(_windowedSize.x), static_cast<float>(_windowedSize.y)));
        _window->setView(sf::View(visibleArea));
        
        // Notify renderer of size change
        std::stringstream resizeMsg;
        resizeMsg << _windowedSize.x << "," << _windowedSize.y;
        sendMessage("WindowResized", resizeMsg.str());
        
        std::cout << "[SFMLWindowManager] Switched to windowed: " << _windowedSize.x << "x" << _windowedSize.y << std::endl;
    }
    
    // Send fullscreen state change notification
    sendMessage("FullscreenChanged", _isFullscreen ? "1" : "0");
}

}  // namespace rtypeEngine

extern "C" SFML_WINDOW_MANAGER_EXPORT rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::SFMLWindowManager(pubEndpoint, subEndpoint);
}
