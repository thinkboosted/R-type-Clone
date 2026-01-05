#pragma once

#include <string>
#include "../AModule.hpp"

namespace rtypeEngine {

/**
 * @brief Interface for sound management modules.
 * 
 * Handles playing sounds (one-shot effects) and music (looped background tracks).
 * 
 * Message topics this module subscribes to:
 * - "SoundPlay"       : Play a one-shot sound effect. Format: "<soundId>:<filePath>" or "<soundId>:<filePath>:<volume>"
 * - "SoundStop"       : Stop a currently playing sound. Format: "<soundId>"
 * - "MusicPlay"       : Play background music (loops). Format: "<musicId>:<filePath>" or "<musicId>:<filePath>:<volume>"
 * - "MusicStop"       : Stop background music. Format: "<musicId>"
 * - "MusicPause"      : Pause background music. Format: "<musicId>"
 * - "MusicResume"     : Resume paused music. Format: "<musicId>"
 * - "SoundSetVolume"  : Set volume for a sound. Format: "<soundId>:<volume>" (0-100)
 * - "MusicSetVolume"  : Set volume for music. Format: "<musicId>:<volume>" (0-100)
 * - "SoundStopAll"    : Stop all sounds. No payload needed.
 * - "MusicStopAll"    : Stop all music. No payload needed.
 */
class ISoundManager : public AModule {
  public:
    using AModule::AModule;
    virtual ~ISoundManager() = default;

  protected:
    // Sound effects (short, can overlap)
    virtual void playSound(const std::string& soundId, const std::string& filePath, float volume = 100.0f) = 0;
    virtual void stopSound(const std::string& soundId) = 0;
    virtual void setSoundVolume(const std::string& soundId, float volume) = 0;

    // Music (long, streamed, looped by default)
    virtual void playMusic(const std::string& musicId, const std::string& filePath, float volume = 100.0f, bool loop = true) = 0;
    virtual void stopMusic(const std::string& musicId) = 0;
    virtual void pauseMusic(const std::string& musicId) = 0;
    virtual void resumeMusic(const std::string& musicId) = 0;
    virtual void setMusicVolume(const std::string& musicId, float volume) = 0;

    // Global controls
    virtual void stopAllSounds() = 0;
    virtual void stopAllMusic() = 0;
};

}  // namespace rtypeEngine
