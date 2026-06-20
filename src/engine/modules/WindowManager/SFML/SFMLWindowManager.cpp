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
#include <zmq.hpp>

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
        this->handleSetFullscreen(message);
    });
    subscribe("SetWindowSize", [this](const std::string& message) {
        this->handleSetWindowSize(message);
    });
    subscribe("ToggleFullscreen", [this](const std::string&) {
        this->recreateWindow(!_isFullscreen);
    });
    subscribe("GetWindowInfo", [this](const std::string& message) {
        this->handleGetWindowInfo(message);
    });
}

void SFMLWindowManager::loop() {
    if (_window && _window->isOpen()) {
        while (auto event = _window->pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                _window->close();
                sendMessage("ExitApplication", "");
            }

            if (event->is<sf::Event::FocusLost>()) {
                _isFocused = false;
                // Prevent sticky movement when window loses focus while key is held.
                for (int k = 0; k < sf::Keyboard::KeyCount; ++k) {
                    if (_pressedKeys[k]) {
                        auto it = keyMappings.find(static_cast<sf::Keyboard::Key>(k));
                        if (it != keyMappings.end()) {
                            sendMessage("KeyReleased", it->second);
                        }
                        _pressedKeys[k] = false;
                    }
                }
                continue;
            }

            if (event->is<sf::Event::FocusGained>()) {
                _isFocused = true;
                continue;
            }

            if (!_isFocused) {
                continue;
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
                _pressedKeys[static_cast<int>(keyReleased->code)] = false;
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                auto it = keyMappings.find(keyPressed->code);
                if (it != keyMappings.end()) {
                    const int keyIndex = static_cast<int>(keyPressed->code);
                    if (!_pressedKeys[keyIndex]) {
                        sendMessage("KeyPressed", it->second);
                        _pressedKeys[keyIndex] = true;
                    }
                }
            }
            if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                // OPTIMIZATION: Only process if size actually changed
                thread_local unsigned int lastWidth = 0;
                thread_local unsigned int lastHeight = 0;
                thread_local auto lastResizeTime = std::chrono::steady_clock::now();

                // Skip duplicate events
                if (resized->size.x == lastWidth && resized->size.y == lastHeight) {
                    continue;
                }

                // Throttle rapid resize events to ~60fps (16ms interval)
                constexpr auto MIN_RESIZE_INTERVAL_MS = 16; // ~60fps throttling interval in milliseconds
                auto currentTime = std::chrono::steady_clock::now();
                auto timeSinceLastResize = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastResizeTime).count();

                if (timeSinceLastResize < MIN_RESIZE_INTERVAL_MS && lastWidth != 0) {
                    continue;
                }

                lastWidth = resized->size.x;
                lastHeight = resized->size.y;
                lastResizeTime = currentTime;

                std::stringstream ss;
                ss << resized->size.x << "," << resized->size.y;
                sendMessage("WindowResized", ss.str());

                // Only recreate texture if size actually changed
                if (_texture.getSize().x != resized->size.x || _texture.getSize().y != resized->size.y) {
                    _texture = sf::Texture(sf::Vector2u(resized->size.x, resized->size.y));
                    _sprite = sf::Sprite(_texture);
                    _sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(static_cast<int>(resized->size.x), static_cast<int>(resized->size.y))));
                    _sprite.setPosition(sf::Vector2f(0, 0));
                    _sprite.setScale(sf::Vector2f(1.0f, 1.0f));
                }

                sf::FloatRect visibleArea(sf::Vector2f(0, 0), sf::Vector2f(static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)));
                _window->setView(sf::View(visibleArea));

                // Update windowed size if not fullscreen
                if (!_isFullscreen) {
                    _windowedSize = {resized->size.x, resized->size.y};
                }
            }
        }
    }
}

void SFMLWindowManager::cleanup() {
    if (_window && _window->isOpen()) {
        _window->close();
    }
}

void SFMLWindowManager::processMessages() {
    std::string latestFrame;
    bool hasLatestFrame = false;
    int messagesProcessed = 0;
    constexpr int maxMessagesPerLoop = 64;

    while (messagesProcessed < maxMessagesPerLoop) {
        zmq::message_t zmqMessage;
        auto result = _subscriber->recv(zmqMessage, zmq::recv_flags::dontwait);
        if (!result) {
            break;
        }
        ++messagesProcessed;

        std::string fullMessage(static_cast<char*>(zmqMessage.data()), zmqMessage.size());
        constexpr const char* imageTopic = "ImageRendered";
        constexpr std::size_t imageTopicLen = 13;
        if (fullMessage.rfind(imageTopic, 0) == 0 &&
            (fullMessage.size() == imageTopicLen || fullMessage[imageTopicLen] == ' ')) {
            latestFrame = fullMessage.size() > imageTopicLen + 1 ? fullMessage.substr(imageTopicLen + 1) : "";
            hasLatestFrame = true;
            continue;
        }

        for (const auto& subscription : _subscriptions) {
            const std::string& topic = subscription.first;
            const MessageHandler& handler = subscription.second;

            if (fullMessage.find(topic) == 0 &&
                (fullMessage.length() == topic.length() || fullMessage[topic.length()] == ' ')) {
                std::string payload;
                if (fullMessage.length() > topic.length() + 1) {
                    payload = fullMessage.substr(topic.length() + 1);
                }
                try {
                    handler(payload);
                } catch (const std::exception& e) {
                    std::cerr << "[SFMLWindowManager] Handler error for topic '" << topic << "': " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "[SFMLWindowManager] Handler error for topic '" << topic << "': unknown error" << std::endl;
                }
            }
        }
    }

    if (hasLatestFrame) {
        handleImageRendered(latestFrame);
    }
}

void SFMLWindowManager::createWindow(const std::string &title, const Vector2u &size) {
    _window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode(sf::Vector2u(size.x, size.y)), title);
    _window->setKeyRepeatEnabled(false);
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
    drawPixelBytes(reinterpret_cast<const std::uint8_t*>(pixels.data()), size);
}

void SFMLWindowManager::drawPixelBytes(const std::uint8_t* pixels, const Vector2u &size) {
    if (!_window || !_window->isOpen()) {
        return;
    }

    _window->setActive(true);
    _texture.update(pixels);

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
    const std::size_t bodySize = pixelData.size() - sep - 1;
    const char* bodyData = pixelData.data() + sep + 1;

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

    size_t pixelCount = bodySize / sizeof(uint32_t);
    if (pixelCount != static_cast<size_t>(width) * static_cast<size_t>(height)) {
        std::cerr << "[SFMLWindowManager] handleImageRendered: pixel size mismatch (got " << pixelCount << ", expected " << width * height << ")" << std::endl;
        return;
    }

    // If the texture size doesn't match the incoming image, recreate the texture and sprite
    sf::Vector2u texSize = _texture.getSize();
    if (texSize.x != width || texSize.y != height) {
        _texture = sf::Texture(sf::Vector2u(width, height));
        _sprite = sf::Sprite(_texture);
        _sprite.setTextureRect(sf::IntRect(sf::Vector2i(0,0), sf::Vector2i(static_cast<int>(width), static_cast<int>(height))));
        _sprite.setPosition(sf::Vector2f(0,0));
        _sprite.setScale(sf::Vector2f(1.0f,1.0f));
    }

    drawPixelBytes(reinterpret_cast<const std::uint8_t*>(bodyData), Vector2u{width, height});
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

            // Skip if already at this size
            if (_window && _window->getSize().x == width && _window->getSize().y == height) {
                return;
            }

            if (width > 0 && height > 0 && _window) {
                _windowedSize = {width, height};

                // If in fullscreen, need to recreate for mode change
                if (_isFullscreen) {
                    _isFullscreen = false;
                    recreateWindow(false);
                    return;
                }

                // OPTIMIZATION: Use setSize instead of recreating window
                _window->setSize(sf::Vector2u(width, height));

                // Only recreate texture if size changed
                if (_texture.getSize().x != width || _texture.getSize().y != height) {
                    _texture = sf::Texture(sf::Vector2u(width, height));
                    _sprite = sf::Sprite(_texture);
                    _sprite.setTextureRect(sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(static_cast<int>(width), static_cast<int>(height))));
                    _sprite.setPosition(sf::Vector2f(0, 0));
                    _sprite.setScale(sf::Vector2f(1.0f, 1.0f));
                }

                // Update view for new size
                sf::FloatRect visibleArea(sf::Vector2f(0, 0), sf::Vector2f(static_cast<float>(width), static_cast<float>(height)));
                _window->setView(sf::View(visibleArea));

                // Notify renderer of size change
                std::stringstream resizeMsg;
                resizeMsg << width << "," << height;
                sendMessage("WindowResized", resizeMsg.str());
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
        _window->setKeyRepeatEnabled(false);

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
    } else {
        // Windowed mode
        _window = std::make_unique<sf::RenderWindow>(
            sf::VideoMode(sf::Vector2u(_windowedSize.x, _windowedSize.y)), _windowTitle, sf::Style::Default);
        _window->setKeyRepeatEnabled(false);

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
    }

    // Send fullscreen state change notification
    sendMessage("FullscreenChanged", _isFullscreen ? "1" : "0");
}

}  // namespace rtypeEngine

extern "C" SFML_WINDOW_MANAGER_EXPORT rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::SFMLWindowManager(pubEndpoint, subEndpoint);
}
