#pragma once

#include <string>
#include "../AModule.hpp"

namespace rtypeEngine {

class ISoundManager : public AModule {
  public:
    using AModule::AModule;
    virtual ~ISoundManager() = default;

  protected:
    virtual void playSound(const std::string& soundId, const std::string& filePath, float volume = 100.0f) = 0;
    virtual void stopSound(const std::string& soundId) = 0;
    virtual void setSoundVolume(const std::string& soundId, float volume) = 0;

    virtual void playMusic(const std::string& musicId, const std::string& filePath, float volume = 100.0f, bool loop = true) = 0;
    virtual void stopMusic(const std::string& musicId) = 0;
    virtual void pauseMusic(const std::string& musicId) = 0;
    virtual void resumeMusic(const std::string& musicId) = 0;
    virtual void setMusicVolume(const std::string& musicId, float volume) = 0;

    virtual void stopAllSounds() = 0;
    virtual void stopAllMusic() = 0;
};

}  // namespace rtypeEngine
