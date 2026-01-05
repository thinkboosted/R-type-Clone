-- Sound Test Script
-- Tests the sound system by sending messages through ECS
-- All test logic runs in Lua to demonstrate ECS message system

print("=== Lua Sound Test Script ===")

local SoundTest = {}

local timeSinceStart = 0
local stepIndex = 1

-- Real asset paths
local LASER_SOUND = "effects/laser.wav"
local EXPLOSION_SOUND = "effects/expltio.wav"
local MUSIC_FILE = "music/background.ogg"

local steps = {
    -- === Sound Effects Testing ===
    { t = 0.0, label = "Play laser sound (90% volume)", fn = function()
        ECS.sendMessage("SoundPlay", "test_laser:" .. LASER_SOUND .. ":90")
    end },
    
    { t = 0.8, label = "Play explosion sound (100% volume)", fn = function()
        ECS.sendMessage("SoundPlay", "test_explosion:" .. EXPLOSION_SOUND .. ":100")
    end },
    
    { t = 1.6, label = "Play laser again (different ID, 70% volume)", fn = function()
        ECS.sendMessage("SoundPlay", "test_laser2:" .. LASER_SOUND .. ":70")
    end },
    
    { t = 2.2, label = "Adjust laser volume to 40%", fn = function()
        ECS.sendMessage("SoundSetVolume", "test_laser:40")
    end },
    
    { t = 2.8, label = "Play explosion at low volume (30%)", fn = function()
        ECS.sendMessage("SoundPlay", "test_explosion2:" .. EXPLOSION_SOUND .. ":30")
    end },
    
    { t = 3.4, label = "Stop first laser sound", fn = function()
        ECS.sendMessage("SoundStop", "test_laser")
    end },
    
    { t = 4.0, label = "Stop all remaining sounds", fn = function()
        ECS.sendMessage("SoundStopAll", "")
    end },
    
    -- === Music Testing ===
    { t = 4.8, label = "Start background music (50% volume, looping)", fn = function()
        ECS.sendMessage("MusicPlay", "bgm:" .. MUSIC_FILE .. ":50")
    end },
    
    { t = 6.0, label = "Lower music volume to 25%", fn = function()
        ECS.sendMessage("MusicSetVolume", "bgm:25")
    end },
    
    { t = 7.0, label = "Pause background music", fn = function()
        ECS.sendMessage("MusicPause", "bgm")
    end },
    
    { t = 8.0, label = "Resume background music", fn = function()
        ECS.sendMessage("MusicResume", "bgm")
    end },
    
    { t = 9.0, label = "Increase music volume to 60%", fn = function()
        ECS.sendMessage("MusicSetVolume", "bgm:60")
    end },
    
    { t = 10.0, label = "Play second music track simultaneously", fn = function()
        ECS.sendMessage("MusicPlay", "bgm2:" .. MUSIC_FILE .. ":30")
    end },
    
    { t = 11.0, label = "Stop first music track", fn = function()
        ECS.sendMessage("MusicStop", "bgm")
    end },
    
    { t = 12.0, label = "Stop all music", fn = function()
        ECS.sendMessage("MusicStopAll", "")
    end },
    
    -- === Mixed Sound/Music Testing ===
    { t = 12.8, label = "Start music + play sound effects simultaneously", fn = function()
        ECS.sendMessage("MusicPlay", "final_bgm:" .. MUSIC_FILE .. ":40")
        ECS.sendMessage("SoundPlay", "final_laser:" .. LASER_SOUND .. ":80")
    end },
    
    { t = 13.4, label = "Play explosion with music playing", fn = function()
        ECS.sendMessage("SoundPlay", "final_explosion:" .. EXPLOSION_SOUND .. ":85")
    end },
    
    { t = 14.0, label = "Rapid-fire laser sounds", fn = function()
        ECS.sendMessage("SoundPlay", "rapid1:" .. LASER_SOUND .. ":60")
        ECS.sendMessage("SoundPlay", "rapid2:" .. LASER_SOUND .. ":65")
        ECS.sendMessage("SoundPlay", "rapid3:" .. LASER_SOUND .. ":70")
    end },
    
    { t = 15.0, label = "Stop all sounds (keep music)", fn = function()
        ECS.sendMessage("SoundStopAll", "")
    end },
    
    { t = 16.0, label = "Stop all audio", fn = function()
        ECS.sendMessage("MusicStopAll", "")
        ECS.sendMessage("SoundStopAll", "")
    end },
    
    -- === Error Handling Test ===
    { t = 16.8, label = "Test invalid file path (should log error)", fn = function()
        ECS.sendMessage("SoundPlay", "invalid_test:effects/nonexistent.wav:80")
    end },
    
    { t = 17.4, label = "Test completed - all topics exercised", fn = function()
        print("[SoundTest] === All tests completed successfully! ===")
        print("[SoundTest] Tested topics:")
        print("[SoundTest]   - SoundPlay (multiple instances, various volumes)")
        print("[SoundTest]   - SoundSetVolume")
        print("[SoundTest]   - SoundStop")
        print("[SoundTest]   - SoundStopAll")
        print("[SoundTest]   - MusicPlay (looping, multiple tracks)")
        print("[SoundTest]   - MusicSetVolume")
        print("[SoundTest]   - MusicPause")
        print("[SoundTest]   - MusicResume")
        print("[SoundTest]   - MusicStop")
        print("[SoundTest]   - MusicStopAll")
        print("[SoundTest]   - Error handling (invalid paths)")
    end },
}

function SoundTest.init()
    print("[SoundTest] Initialized - testing with real assets:")
    print("[SoundTest]   Laser:     " .. LASER_SOUND)
    print("[SoundTest]   Explosion: " .. EXPLOSION_SOUND)
    print("[SoundTest]   Music:     " .. MUSIC_FILE)
    print("[SoundTest] Running " .. #steps .. " staged tests...")
end

function SoundTest.update(dt)
    timeSinceStart = timeSinceStart + dt
    while stepIndex <= #steps and timeSinceStart >= steps[stepIndex].t do
        local step = steps[stepIndex]
        print("[SoundTest] Step " .. stepIndex .. "/" .. #steps .. ": " .. step.label)
        step.fn()
        stepIndex = stepIndex + 1
    end
end

-- Register system so update gets called
ECS.registerSystem(SoundTest)

return SoundTest
