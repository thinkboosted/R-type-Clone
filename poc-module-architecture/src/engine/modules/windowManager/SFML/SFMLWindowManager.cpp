#include "SFMLWindowManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace rtypeEngine {

SFMLWindowManager::SFMLWindowManager(const char* pubEndpoint, const char* subEndpoint)
    : IWindowManager(pubEndpoint, subEndpoint), _window(nullptr), _texture(sf::Vector2u(1, 1)), _sprite(_texture) {}

void SFMLWindowManager::init() {
    createWindow("SFML Window", Vector2u{800, 600});
    subscribe("ImageRendered", [this](const std::string& message) {
        this->handleImageRendered(message);
    });
    subscribe("CloseWindow", [this](const std::string&) {
        this->close();
    });
}

void SFMLWindowManager::loop() {
    if (_window && _window->isOpen()) {
        while (auto event = _window->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window->close();
                sendMessage("ExitApplication", "");
            }
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
    const uint32_t* pixelPtr = reinterpret_cast<const uint32_t*>(pixelData.data());
    size_t pixelCount = pixelData.size() / sizeof(uint32_t);

    std::vector<uint32_t> pixels(pixelPtr, pixelPtr + pixelCount);

    drawPixels(pixels, Vector2u{800, 600});
}

}  // namespace rtypeEngine

extern "C" __declspec(dllexport) rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::SFMLWindowManager(pubEndpoint, subEndpoint);
}
