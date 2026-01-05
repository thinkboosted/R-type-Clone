# Sound Assets

This directory contains all sound effects and music for the R-Type Clone game.

## Directory Structure

```
assets/sounds/
├── effects/           # Short sound effects (WAV format recommended)
│   ├── laser.wav      # Player shooting sound
│   ├── explosion.wav  # Enemy explosion sound
│   ├── hit.wav        # Player hit/damage sound
│   ├── powerup.wav    # Bonus collection sound
│   └── gameover.wav   # Game over sound
│
└── music/             # Background music (OGG format recommended)
    └── background.ogg # Main game background music (loops)
```

## Required Sound Files

### Sound Effects (`effects/`)
| File           | Description                        | When Played                    |
|----------------|------------------------------------|---------------------------------|
| `laser.wav`    | Player shooting/firing             | When player presses SPACE      |
| `explosion.wav`| Enemy death explosion              | When bullet hits enemy         |
| `hit.wav`      | Player taking damage               | When player collides with enemy|
| `powerup.wav`  | Collecting bonus/power-up          | When player picks up bonus     |
| `gameover.wav` | Game over jingle                   | When player dies               |

### Music (`music/`)
| File             | Description                      | When Played              |
|------------------|----------------------------------|--------------------------|
| `background.ogg` | Main gameplay music              | When game starts (loops) |

## Supported Formats

- **Sound Effects**: WAV (recommended), OGG, FLAC
- **Music**: OGG (recommended for smaller file size), WAV, FLAC

## Usage in Lua Scripts

```lua
-- Play a sound effect (format: "id:path:volume")
ECS.sendMessage("SoundPlay", "shoot:effects/laser.wav:80")

-- Play background music (loops automatically)
ECS.sendMessage("MusicPlay", "bgm:music/background.ogg:50")

-- Stop music
ECS.sendMessage("MusicStop", "bgm")

-- Pause/Resume music
ECS.sendMessage("MusicPause", "bgm")
ECS.sendMessage("MusicResume", "bgm")

-- Set volume (0-100)
ECS.sendMessage("MusicSetVolume", "bgm:30")
ECS.sendMessage("SoundSetVolume", "shoot:50")

-- Stop all sounds/music
ECS.sendMessage("SoundStopAll", "")
ECS.sendMessage("MusicStopAll", "")
```

## Where to Get Free Sounds

- [OpenGameArt.org](https://opengameart.org/)
- [Freesound.org](https://freesound.org/)
- [Kenney.nl](https://kenney.nl/assets?q=audio) - Excellent free game audio packs
- [ZapSplat](https://www.zapsplat.com/)

Make sure to check licenses before using any sounds in your project.
