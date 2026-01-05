#pragma once

#include <SFML/Audio.hpp>
#include <memory>
#include <unordered_map>
#include <string>
#include "../ISoundManager.hpp"

namespace rtypeEngine {

class SFMLSoundManager : public ISoundManager {
  public:
    explicit SFMLSoundManager(const char* pubEndpoint, const char* subEndpoint);
    ~SFMLSoundManager() override = default;

    void init() override;
    void loop() override;
    void cleanup() override;

  protected:
    void playSound(const std::string& soundId, const std::string& filePath, float volume = 100.0f) override;
    void stopSound(const std::string& soundId) override;
    void setSoundVolume(const std::string& soundId, float volume) override;

    void playMusic(const std::string& musicId, const std::string& filePath, float volume = 100.0f, bool loop = true) override;
    void stopMusic(const std::string& musicId) override;
    void pauseMusic(const std::string& musicId) override;
    void resumeMusic(const std::string& musicId) override;
    void setMusicVolume(const std::string& musicId, float volume) override;

    void stopAllSounds() override;
    void stopAllMusic() override;

  private:
    void handleSoundPlay(const std::string& message);
    void handleSoundStop(const std::string& message);
    void handleMusicPlay(const std::string& message);
    void handleMusicStop(const std::string& message);
    void handleMusicPause(const std::string& message);
    void handleMusicResume(const std::string& message);
    void handleSoundSetVolume(const std::string& message);
    void handleMusicSetVolume(const std::string& message);

    struct ParsedSoundMessage {
        std::string id;
        std::string path;
        float volume = 100.0f;
    };
    ParsedSoundMessage parseMessage(const std::string& message);

    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> _soundBuffers;
    std::unordered_map<std::string, std::unique_ptr<sf::Sound>> _activeSounds;
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> _activeMusic;
    std::string _assetsPath = "assets/sounds/";
};

}  // namespace rtypeEngine
