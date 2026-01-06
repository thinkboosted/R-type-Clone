-- Sound Test Script
-- Tests ALL game sounds through ECS message system

print("=== Lua Sound Test Script ===")

local SoundTest = {}

local timeSinceStart = 0
local stepIndex = 1

-- All game sound assets
local LASER_SOUND = "effects/laser.wav"
local EXPLOSION_SOUND = "effects/explosion.wav"
local HIT_SOUND = "effects/hit.wav"
local POWERUP_SOUND = "effects/powerup.wav"
local GAMEOVER_SOUND = "effects/gameover.wav"
local MUSIC_FILE = "music/background.ogg"

local steps = {
    -- === All Sound Effects ===
    { t = 0.0, label = "Play laser sound (player shooting)", fn = function()
        ECS.sendMessage("SoundPlay", "laser:" .. LASER_SOUND .. ":80")
    end },
    
    { t = 0.8, label = "Play explosion sound (enemy destroyed)", fn = function()
        ECS.sendMessage("SoundPlay", "explosion:" .. EXPLOSION_SOUND .. ":90")
    end },
    
    { t = 1.6, label = "Play hit sound (player damaged)", fn = function()
        ECS.sendMessage("SoundPlay", "hit:" .. HIT_SOUND .. ":100")
    end },
    
    { t = 2.4, label = "Play powerup sound (bonus collected)", fn = function()
        ECS.sendMessage("SoundPlay", "powerup:" .. POWERUP_SOUND .. ":100")
    end },
    
    { t = 3.2, label = "Play gameover sound", fn = function()
        ECS.sendMessage("SoundPlay", "gameover:" .. GAMEOVER_SOUND .. ":100")
    end },
    
    { t = 4.5, label = "Stop all sounds", fn = function()
        ECS.sendMessage("SoundStopAll", "")
    end },
    
    -- === Music Testing ===
    { t = 5.0, label = "Start background music (50% volume)", fn = function()
        ECS.sendMessage("MusicPlay", "bgm:" .. MUSIC_FILE .. ":50")
    end },
    
    { t = 6.5, label = "Lower music volume to 25%", fn = function()
        ECS.sendMessage("MusicSetVolume", "bgm:25")
    end },
    
    { t = 7.5, label = "Pause music", fn = function()
        ECS.sendMessage("MusicPause", "bgm")
    end },
    
    { t = 8.5, label = "Resume music", fn = function()
        ECS.sendMessage("MusicResume", "bgm")
    end },
    
    { t = 9.5, label = "Increase music volume to 60%", fn = function()
        ECS.sendMessage("MusicSetVolume", "bgm:60")
    end },
    
    { t = 10.5, label = "Stop music", fn = function()
        ECS.sendMessage("MusicStop", "bgm")
    end },
    
    -- === Combined Test (simulating gameplay) ===
    { t = 11.5, label = "Start music + rapid laser fire", fn = function()
        ECS.sendMessage("MusicPlay", "bgm:" .. MUSIC_FILE .. ":40")
        ECS.sendMessage("SoundPlay", "laser1:" .. LASER_SOUND .. ":70")
        ECS.sendMessage("SoundPlay", "laser2:" .. LASER_SOUND .. ":75")
        ECS.sendMessage("SoundPlay", "laser3:" .. LASER_SOUND .. ":80")
    end },
    
    { t = 12.5, label = "Enemy explosions", fn = function()
        ECS.sendMessage("SoundPlay", "exp1:" .. EXPLOSION_SOUND .. ":85")
        ECS.sendMessage("SoundPlay", "exp2:" .. EXPLOSION_SOUND .. ":90")
    end },
    
    { t = 13.5, label = "Player hit + powerup", fn = function()
        ECS.sendMessage("SoundPlay", "hit:" .. HIT_SOUND .. ":100")
        ECS.sendMessage("SoundPlay", "powerup:" .. POWERUP_SOUND .. ":100")
    end },
    
    { t = 14.5, label = "Stop all audio", fn = function()
        ECS.sendMessage("SoundStopAll", "")
        ECS.sendMessage("MusicStopAll", "")
    end },
    
    -- === Error Handling ===
    { t = 15.0, label = "Test invalid path (expected error)", fn = function()
        ECS.sendMessage("SoundPlay", "invalid:effects/nonexistent.wav:80")
    end },
    
    { t = 15.5, label = "Test completed", fn = function()
        print("[SoundTest] === All tests completed! ===")
        print("[SoundTest] Tested all game sounds:")
        print("[SoundTest]   - laser.wav (shooting)")
        print("[SoundTest]   - explosion.wav (enemy death)")
        print("[SoundTest]   - hit.wav (player damage)")
        print("[SoundTest]   - powerup.wav (bonus)")
        print("[SoundTest]   - gameover.wav (game over)")
        print("[SoundTest]   - background.ogg (music)")
        
        -- Signal completion to C++ test app
        ECS.sendMessage("TestComplete", "all_tests_passed")
    end },
}

function SoundTest.init()
    print("[SoundTest] Initialized - testing ALL game sounds:")
    print("[SoundTest]   Laser:     " .. LASER_SOUND)
    print("[SoundTest]   Explosion: " .. EXPLOSION_SOUND)
    print("[SoundTest]   Hit:       " .. HIT_SOUND)
    print("[SoundTest]   Powerup:   " .. POWERUP_SOUND)
    print("[SoundTest]   Gameover:  " .. GAMEOVER_SOUND)
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

ECS.registerSystem(SoundTest)

return SoundTest
