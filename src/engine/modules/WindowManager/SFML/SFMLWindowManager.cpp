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
    : IWindowManager(pubEndpoint, subEndpoint), _window(nullptr), _texture(sf::Vector2u(1, 1)), _sprite(_texture) {}

void SFMLWindowManager::init() {
    createWindow("R-Type Engine - SFML Window", Vector2u{800, 600});
    subscribe("ImageRendered", [this](const std::string& message) {
        this->handleImageRendered(message);
    });
    subscribe("CloseWindow", [this](const std::string&) {
        this->close();
    });
    subscribe("PipelinePhase", [this](const std::string& phase) {
        if (phase == "RENDER" && _window && _window->isOpen()) {
            _window->clear(sf::Color::Black);
            _window->draw(_sprite);
            _window->display();
        }
    });
    std::cout << "[SFMLWindowManager] Initialized with window 800x600" << std::endl;
}

// ═══════════════════════════════════════════════════════════════
// HARD-WIRED FIXED UPDATE (Called by GameEngine at 60Hz)
// ═══════════════════════════════════════════════════════════════
void SFMLWindowManager::fixedUpdate(double /*dt*/) {
    if (!_window || !_window->isOpen()) {
        return;
    }

    // Poll all events from SFML
    while (auto event = _window->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            std::cout << "[SFMLWindowManager] Window closed by user" << std::endl;
            _window->close();
            sendMessage("ExitApplication", "");
            return;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                std::cout << "[SFMLWindowManager] Escape pressed - closing window" << std::endl;
                _window->close();
                sendMessage("ExitApplication", "");
                return;
            }
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            std::stringstream ss;
            ss << static_cast<int>(mousePressed->button) << ":" << mousePressed->position.x << "," << mousePressed->position.y;
            sendMessage("MousePressed", ss.str());
        }

        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
            std::stringstream ss;
            ss << mouseMoved->position.x << "," << mouseMoved->position.y;
            sendMessage("MouseMoved", ss.str());
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// LEGACY LOOP (DEPRECATED - kept for backward compatibility)
// ═══════════════════════════════════════════════════════════════
void SFMLWindowManager::loop() {
    if (_window && _window->isOpen()) {
        while (auto event = _window->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window->close();
                sendMessage("ExitApplication", "");
                return;
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    _window->close();
                    std::cout << "[SFMLWindowManager] closing window." << std::endl;
                    sendMessage("ExitApplication", "");
                    return;
                }
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

                sf::FloatRect visibleArea(sf::Vector2f(0, 0), sf::Vector2f(resized->size.x, resized->size.y));
                _window->setView(sf::View(visibleArea));
            }
        }
    }

    for (int k = 0; k < sf::Keyboard::KeyCount; ++k) {
        auto key = static_cast<sf::Keyboard::Key>(k);
        if (sf::Keyboard::isKeyPressed(key)) {
            auto it = keyMappings.find(key);
            if (it != keyMappings.end()) {
                sendMessage("KeyPressed", it->second);
            }
        }
    }
}

void SFMLWindowManager::render(double /*alpha*/) {
    if (_window && _window->isOpen()) {
        // Activate context for rendering in this thread
        _window->setActive(true);
        _window->display();
        _window->setActive(false);  // Release for next frame
    }
}

void SFMLWindowManager::cleanup() {
    if (_window && _window->isOpen()) {
        _window->close();
    }
}

void SFMLWindowManager::createWindow(const std::string &title, const Vector2u &size) {
    // Configure OpenGL context settings to avoid conflicts
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antiAliasingLevel = 0;  // Disable antialiasing to reduce conflicts
    settings.majorVersion = 2;       // Request OpenGL 2.1 (compatible)
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Default;

    _window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode(sf::Vector2u(size.x, size.y)), title, sf::Style::Default, sf::State::Windowed, settings);

    // Deactivate context in this thread to allow rendering thread to use it
    _window->setActive(false);

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

    if (!pixels.empty()) {
        _texture.update(reinterpret_cast<const std::uint8_t*>(pixels.data()));
    }

    _window->clear(sf::Color::Black);
    _window->draw(_sprite);
    _window->display();
}

void SFMLWindowManager::handleImageRendered(const std::string& pixelData) {
    const uint32_t* pixelPtr = reinterpret_cast<const uint32_t*>(pixelData.data());
    size_t pixelCount = pixelData.size() / sizeof(uint32_t);

    sf::Vector2u texSize = _texture.getSize();
    if (pixelCount != texSize.x * texSize.y) {
        return;
    }

    std::vector<uint32_t> pixels(pixelPtr, pixelPtr + pixelCount);

    drawPixels(pixels, Vector2u{texSize.x, texSize.y});
}

}  // namespace rtypeEngine

extern "C" SFML_WINDOW_MANAGER_EXPORT rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::SFMLWindowManager(pubEndpoint, subEndpoint);
}
