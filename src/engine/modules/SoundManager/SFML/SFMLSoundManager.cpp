#ifdef _WIN32
    #define SFML_SOUND_MANAGER_EXPORT __declspec(dllexport)
#else
    #define SFML_SOUND_MANAGER_EXPORT
#endif

#include "SFMLSoundManager.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace rtypeEngine {

SFMLSoundManager::SFMLSoundManager(const char* pubEndpoint, const char* subEndpoint)
    : ISoundManager(pubEndpoint, subEndpoint) {}

void SFMLSoundManager::init() {
    // Subscribe to sound-related messages
    subscribe("SoundPlay", [this](const std::string& message) {
        this->handleSoundPlay(message);
    });
    subscribe("SoundStop", [this](const std::string& message) {
        this->handleSoundStop(message);
    });
    subscribe("SoundSetVolume", [this](const std::string& message) {
        this->handleSoundSetVolume(message);
    });
    subscribe("SoundStopAll", [this](const std::string&) {
        this->stopAllSounds();
    });

    // Subscribe to music-related messages
    subscribe("MusicPlay", [this](const std::string& message) {
        this->handleMusicPlay(message);
    });
    subscribe("MusicStop", [this](const std::string& message) {
        this->handleMusicStop(message);
    });
    subscribe("MusicPause", [this](const std::string& message) {
        this->handleMusicPause(message);
    });
    subscribe("MusicResume", [this](const std::string& message) {
        this->handleMusicResume(message);
    });
    subscribe("MusicSetVolume", [this](const std::string& message) {
        this->handleMusicSetVolume(message);
    });
    subscribe("MusicStopAll", [this](const std::string&) {
        this->stopAllMusic();
    });

    std::cout << "[SFMLSoundManager] Initialized" << std::endl;
}

void SFMLSoundManager::loop() {
    for (auto it = _activeSounds.begin(); it != _activeSounds.end();) {
        if (it->second && it->second->getStatus() == sf::Sound::Status::Stopped) {
            it = _activeSounds.erase(it);
        } else {
            ++it;
        }
    }
}

void SFMLSoundManager::cleanup() {
    stopAllSounds();
    stopAllMusic();
    _soundBuffers.clear();
    _activeSounds.clear();
    _activeMusic.clear();
    std::cout << "[SFMLSoundManager] Cleaned up" << std::endl;
}


SFMLSoundManager::ParsedSoundMessage SFMLSoundManager::parseMessage(const std::string& message) {
    ParsedSoundMessage result;
    std::stringstream ss(message);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, ':')) {
        tokens.push_back(token);
    }

    if (tokens.size() >= 1) {
        result.id = tokens[0];
    }
    if (tokens.size() >= 2) {
        result.path = tokens[1];
    }
    if (tokens.size() >= 3) {
        try {
            result.volume = std::stof(tokens[2]);
            result.volume = std::clamp(result.volume, 0.0f, 100.0f);
        } catch (...) {
            result.volume = 100.0f;
        }
    }

    return result;
}

void SFMLSoundManager::handleSoundPlay(const std::string& message) {
    auto parsed = parseMessage(message);
    if (!parsed.id.empty() && !parsed.path.empty()) {
        playSound(parsed.id, parsed.path, parsed.volume);
    }
}

void SFMLSoundManager::handleSoundStop(const std::string& message) {
    stopSound(message);
}

void SFMLSoundManager::handleSoundSetVolume(const std::string& message) {
    auto parsed = parseMessage(message);
    if (!parsed.id.empty()) {
        setSoundVolume(parsed.id, parsed.volume);
    }
}

void SFMLSoundManager::playSound(const std::string& soundId, const std::string& filePath, float volume) {
    std::string fullPath = _assetsPath + filePath;
    std::cout << "[SFMLSoundManager] playSound called: id=" << soundId << " path=" << fullPath << " volume=" << volume << std::endl;

    if (_soundBuffers.find(filePath) == _soundBuffers.end()) {
        auto buffer = std::make_unique<sf::SoundBuffer>();
        if (!buffer->loadFromFile(fullPath)) {
            std::cerr << "[SFMLSoundManager] Failed to load sound: " << fullPath << std::endl;
            return;
        }
        std::cout << "[SFMLSoundManager] Sound buffer loaded successfully: " << fullPath << std::endl;
        _soundBuffers[filePath] = std::move(buffer);
    }

    auto sound = std::make_unique<sf::Sound>(*_soundBuffers[filePath]);
    sound->setVolume(volume);
    sound->play();
    std::cout << "[SFMLSoundManager] Sound playing: " << soundId << " (status=" << static_cast<int>(sound->getStatus()) << ")" << std::endl;

    std::string uniqueKey = soundId + "_" + std::to_string(reinterpret_cast<uintptr_t>(sound.get()));
    _activeSounds[uniqueKey] = std::move(sound);
}

void SFMLSoundManager::stopSound(const std::string& soundId) {
    // Stop all sounds matching the soundId prefix
    for (auto& [key, sound] : _activeSounds) {
        if (key.find(soundId) == 0 && sound) {
            sound->stop();
        }
    }
}

void SFMLSoundManager::setSoundVolume(const std::string& soundId, float volume) {
    for (auto& [key, sound] : _activeSounds) {
        if (key.find(soundId) == 0 && sound) {
            sound->setVolume(std::clamp(volume, 0.0f, 100.0f));
        }
    }
}

void SFMLSoundManager::stopAllSounds() {
    for (auto& [key, sound] : _activeSounds) {
        if (sound) {
            sound->stop();
        }
    }
    _activeSounds.clear();
}

void SFMLSoundManager::handleMusicPlay(const std::string& message) {
    auto parsed = parseMessage(message);
    if (!parsed.id.empty() && !parsed.path.empty()) {
        playMusic(parsed.id, parsed.path, parsed.volume, true);
    }
}

void SFMLSoundManager::handleMusicStop(const std::string& message) {
    stopMusic(message);
}

void SFMLSoundManager::handleMusicPause(const std::string& message) {
    pauseMusic(message);
}

void SFMLSoundManager::handleMusicResume(const std::string& message) {
    resumeMusic(message);
}

void SFMLSoundManager::handleMusicSetVolume(const std::string& message) {
    auto parsed = parseMessage(message);
    if (!parsed.id.empty()) {
        setMusicVolume(parsed.id, parsed.volume);
    }
}

void SFMLSoundManager::playMusic(const std::string& musicId, const std::string& filePath, float volume, bool loop) {
    std::string fullPath = _assetsPath + filePath;

    if (_activeMusic.find(musicId) != _activeMusic.end()) {
        _activeMusic[musicId]->stop();
    }

    auto music = std::make_unique<sf::Music>();
    if (!music->openFromFile(fullPath)) {
        std::cerr << "[SFMLSoundManager] Failed to load music: " << fullPath << std::endl;
        return;
    }

    music->setLooping(loop);
    music->setVolume(volume);
    music->play();

    _activeMusic[musicId] = std::move(music);
}

void SFMLSoundManager::stopMusic(const std::string& musicId) {
    auto it = _activeMusic.find(musicId);
    if (it != _activeMusic.end() && it->second) {
        it->second->stop();
        _activeMusic.erase(it);
    }
}

void SFMLSoundManager::pauseMusic(const std::string& musicId) {
    auto it = _activeMusic.find(musicId);
    if (it != _activeMusic.end() && it->second) {
        it->second->pause();
    }
}

void SFMLSoundManager::resumeMusic(const std::string& musicId) {
    auto it = _activeMusic.find(musicId);
    if (it != _activeMusic.end() && it->second) {
        it->second->play();
    }
}

void SFMLSoundManager::setMusicVolume(const std::string& musicId, float volume) {
    auto it = _activeMusic.find(musicId);
    if (it != _activeMusic.end() && it->second) {
        it->second->setVolume(std::clamp(volume, 0.0f, 100.0f));
    }
}

void SFMLSoundManager::stopAllMusic() {
    for (auto& [key, music] : _activeMusic) {
        if (music) {
            music->stop();
        }
    }
    _activeMusic.clear();
}

}  // namespace rtypeEngine

// Module factory function (called by ModulesManager to instantiate the module)
extern "C" SFML_SOUND_MANAGER_EXPORT rtypeEngine::IModule* createModule(const char* pubEndpoint, const char* subEndpoint) {
    return new rtypeEngine::SFMLSoundManager(pubEndpoint, subEndpoint);
}
