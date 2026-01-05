#pragma once

#include <SFML/Audio.hpp>
#include <memory>
#include <unordered_map>
#include <string>
#include "../ISoundManager.hpp"

namespace rtypeEngine {

/**
 * @brief SFML-based implementation of the SoundManager module.
 * 
 * This module handles audio playback using SFML Audio.
 * - Sounds: Short audio clips loaded into memory (sf::SoundBuffer + sf::Sound)
 * - Music: Streamed audio for background music (sf::Music)
 */
class SFMLSoundManager : public ISoundManager {
  public:
    explicit SFMLSoundManager(const char* pubEndpoint, const char* subEndpoint);
    ~SFMLSoundManager() override = default;

    void init() override;
    void loop() override;
    void cleanup() override;

  protected:
    // Sound effects
    void playSound(const std::string& soundId, const std::string& filePath, float volume = 100.0f) override;
    void stopSound(const std::string& soundId) override;
    void setSoundVolume(const std::string& soundId, float volume) override;

    // Music
    void playMusic(const std::string& musicId, const std::string& filePath, float volume = 100.0f, bool loop = true) override;
    void stopMusic(const std::string& musicId) override;
    void pauseMusic(const std::string& musicId) override;
    void resumeMusic(const std::string& musicId) override;
    void setMusicVolume(const std::string& musicId, float volume) override;

    // Global controls
    void stopAllSounds() override;
    void stopAllMusic() override;

  private:
    // Message handlers
    void handleSoundPlay(const std::string& message);
    void handleSoundStop(const std::string& message);
    void handleMusicPlay(const std::string& message);
    void handleMusicStop(const std::string& message);
    void handleMusicPause(const std::string& message);
    void handleMusicResume(const std::string& message);
    void handleSoundSetVolume(const std::string& message);
    void handleMusicSetVolume(const std::string& message);

    // Helper to parse "id:path" or "id:path:volume" format
    struct ParsedSoundMessage {
        std::string id;
        std::string path;
        float volume = 100.0f;
    };
    ParsedSoundMessage parseMessage(const std::string& message);

    // Sound buffers cache (loaded once, reused)
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> _soundBuffers;
    
    // Active sounds (can have multiple playing at once)
    std::unordered_map<std::string, std::unique_ptr<sf::Sound>> _activeSounds;
    
    // Active music streams
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> _activeMusic;

    // Asset base path
    std::string _assetsPath = "assets/sounds/";
};

}  // namespace rtypeEngine
