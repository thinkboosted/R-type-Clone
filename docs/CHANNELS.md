# 📨 Message Channels Reference

This document lists all message channels (topics) used in the R-Type Clone ECS system. Channels are used for pub/sub communication between systems and modules.

---

## 📋 Table of Contents

- [Channel Types](#channel-types)
- [Input Channels](#-input-channels)
- [Network Channels](#-network-channels)
- [Game State Channels](#-game-state-channels)
- [Rendering Channels](#-rendering-channels)
- [Sound Channels](#-sound-channels)
- [Physics Channels](#-physics-channels)
- [System Channels](#-system-channels)
- [Channels by File](#-channels-by-file)

---

## Channel Types

| Type | Description |
|------|-------------|
| `ECS.subscribe()` | Listen to a channel |
| `ECS.sendMessage()` | Send to local module |
| `ECS.sendNetworkMessage()` | Send to server (client only) |
| `ECS.broadcastNetworkMessage()` | Broadcast to all clients (server only) |

---

## 🎮 Input Channels

### `KeyPressed`
**Direction**: Engine → Lua  
**Payload**: Key name string (`"UP"`, `"DOWN"`, `"LEFT"`, `"RIGHT"`, `"SPACE"`, `"ESCAPE"`, etc.)  
**Subscribers**: `InputSystem`, `MenuSystem`

```lua
ECS.subscribe("KeyPressed", function(key)
    if key == "SPACE" then
        -- Handle shoot
    end
end)
```

### `KeyReleased`
**Direction**: Engine → Lua  
**Payload**: Key name string  
**Subscribers**: `InputSystem`

### `MousePressed`
**Direction**: Engine → Lua  
**Payload**: `"x,y,button"` format  
**Subscribers**: `MenuSystem`

### `MouseMoved`
**Direction**: Engine → Lua  
**Payload**: `"x,y"` format  
**Subscribers**: `MenuSystem`

### `WindowResized`
**Direction**: Engine → Lua  
**Payload**: `"width,height"` format  
**Subscribers**: `MenuSystem`

---

## 🌐 Network Channels

### `INPUT`
**Direction**: Client → Server  
**Payload**: MsgPack `{k="KEY", s=1/0}` or text `"KEY 1"`  
**Purpose**: Send player input to server

```lua
-- Client sends input
ECS.sendBinary("INPUT", {k="UP", s=1})
-- or
ECS.sendNetworkMessage("INPUT", "UP 1")
```

### `ENTITY_POS`
**Direction**: Server → Clients  
**Payload**: MsgPack with entity state
```lua
{
    id = "entity_uuid",
    x = 0.0, y = 0.0, z = 0.0,      -- Position
    rx = 0.0, ry = 0.0, rz = 0.0,   -- Rotation
    vx = 0.0, vy = 0.0, vz = 0.0,   -- Velocity
    t = 1                            -- Type (1=Player, 2=Bullet, 3=Enemy)
}
```

### `ENTITY_DESTROY`
**Direction**: Server → Clients  
**Payload**: Entity ID string  
**Purpose**: Notify clients to destroy an entity

### `ENTITY_HIT`
**Direction**: Server → Clients  
**Payload**: Entity ID string  
**Purpose**: Trigger hit flash effect on client

### `PLAYER_ASSIGN`
**Direction**: Server → Client  
**Payload**: Player entity ID  
**Purpose**: Assign player entity to connected client

### `CLIENT_READY`
**Direction**: Client → Server  
**Payload**: `"ready"`  
**Purpose**: Client confirms it's ready to receive game state

### `CLIENT_RESET`
**Direction**: Server → Client  
**Payload**: Empty  
**Purpose**: Tell client to reset game state

### `ClientConnected`
**Direction**: Engine → Lua (Server)  
**Payload**: Client ID  
**Subscribers**: `NetworkSystem`

### `ClientDisconnected`
**Direction**: Engine → Lua (Server)  
**Payload**: Client ID  
**Subscribers**: `NetworkSystem`

---

## 🎯 Game State Channels

### `REQUEST_GAME_STATE_CHANGE`
**Direction**: Internal  
**Payload**: Target state (`"MENU"`, `"PLAYING"`, `"GAMEOVER"`)  
**Subscribers**: `GameStateManager`

### `GAME_STARTED`
**Direction**: Internal  
**Payload**: Empty  
**Purpose**: Notify systems that game has started

### `GAME_RESET`
**Direction**: Internal  
**Payload**: Empty  
**Purpose**: Notify systems to reset state

### `RESET_GAME`
**Direction**: Client → Server  
**Payload**: Empty  
**Purpose**: Request game reset

### `REQUEST_GAME_START`
**Direction**: Client → Server  
**Payload**: `"1"`  
**Purpose**: Request to start the game

### `GAME_OVER`
**Direction**: Internal  
**Payload**: Final score  
**Purpose**: Signal game over state

### `GAME_SCORE`
**Direction**: Server → Clients  
**Payload**: Score value string  
**Purpose**: Sync score to clients

### `LEVEL_CHANGE`
**Direction**: Server → Clients  
**Payload**: Level number string  
**Purpose**: Notify level change

### `PAUSE_GAME`
**Direction**: Internal  
**Payload**: Empty  
**Subscribers**: `MenuSystem`

### `RESUME_GAME`
**Direction**: Internal  
**Payload**: Empty  
**Subscribers**: `MenuSystem`

### `GAME_PAUSED`
**Direction**: Internal  
**Payload**: Empty  
**Purpose**: Notify systems game is paused

### `GAME_RESUMED`
**Direction**: Internal  
**Payload**: Empty  
**Purpose**: Notify systems game resumed

---

## 🎨 Rendering Channels

### `RenderEntityCommand`
**Direction**: Lua → Renderer Module  
**Payload**: Command string (see formats below)

**Commands**:
```lua
-- Create entity
"CreateEntity:mesh:id"
"CreateEntity:mesh:texturePath:id"

-- Transform
"SetPosition:id,x,y,z"
"SetRotation:id,rx,ry,rz"
"SetScale:id,sx,sy,sz"

-- Appearance
"SetColor:id,r,g,b"
"SetTexture:id:texturePath"

-- Camera
"SetActiveCamera:id"

-- Particles
"CreateParticleGenerator:id,offsetX,offsetY,offsetZ,..."
```

---

## 🔊 Sound Channels

### `SoundPlay`
**Direction**: Lua → Sound Module  
**Payload**: `"id:path:volume"`  
**Example**: `"laser_1:effects/laser.wav:80"`

### `SoundStopAll`
**Direction**: Lua → Sound Module  
**Payload**: Empty

### `MusicPlay`
**Direction**: Lua → Sound Module  
**Payload**: `"id:path:volume"`  
**Example**: `"bgm:music/background.ogg:40"`

### `MusicStop`
**Direction**: Lua → Sound Module  
**Payload**: Music ID

### `MusicPause`
**Direction**: Lua → Sound Module  
**Payload**: Music ID

### `MusicResume`
**Direction**: Lua → Sound Module  
**Payload**: Music ID

### `MusicSetVolume`
**Direction**: Lua → Sound Module  
**Payload**: `"id:volume"`

### `MusicStopAll`
**Direction**: Lua → Sound Module  
**Payload**: Empty

### `PLAY_SOUND`
**Direction**: Server → Clients  
**Payload**: Same as `SoundPlay`

### `STOP_MUSIC`
**Direction**: Server → Clients  
**Payload**: Music ID

---

## ⚙️ Physics Channels

### `PhysicCommand`
**Direction**: Lua → Physics Module  
**Payload**: Command string

**Commands**:
```lua
-- Body management
"CreateBody:id,mass,friction,fixedRotation,useGravity;"
"DestroyBody:id;"

-- Forces
"ApplyForce:id,x,y,z;"
"ApplyImpulse:id,x,y,z;"

-- Transform
"SetPosition:id:x,y,z;"
"SetRotation:id:rx,ry,rz;"
"SetVelocity:id:vx,vy,vz;"
"SetAngularFactor:id:x,y,z;"
```

### `Collision`
**Direction**: Physics Module → Lua  
**Payload**: `"id1:id2"`  
**Subscribers**: `CollisionSystem`

---

## 🔧 System Channels

### `ExitApplication`
**Direction**: Lua → Engine  
**Payload**: Empty  
**Purpose**: Request application exit

### `RESET_DIFFICULTY`
**Direction**: Internal  
**Payload**: Empty  
**Subscribers**: `EnemySystem`

### `REQUEST_SPAWN`
**Direction**: Client → Server  
**Payload**: Client ID  
**Purpose**: Request player spawn

### `SERVER_PLAYER_DEAD`
**Direction**: Internal (Server)  
**Payload**: Client ID  
**Purpose**: Handle player death on server

### `ENEMY_DEAD`
**Direction**: Server → Clients  
**Payload**: `"x,y,z,id"`  
**Purpose**: Notify enemy death for effects

### `TestComplete`
**Direction**: Test → Engine  
**Payload**: Test result  
**Purpose**: Signal test completion

---

## 📁 Channels by File

### `InputSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `KeyPressed` | Subscribe | Handle key press |
| `KeyReleased` | Subscribe | Handle key release |
| `INPUT` | Send (Network) | Forward input to server |

### `NetworkSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `ClientConnected` | Subscribe | Handle new client |
| `ClientDisconnected` | Subscribe | Handle client disconnect |
| `CLIENT_READY` | Subscribe | Client ready acknowledgment |
| `RESET_GAME` | Subscribe | Handle reset request |
| `GAME_STARTED` | Subscribe | Handle game start |
| `REQUEST_GAME_START` | Subscribe | Handle start request |
| `REQUEST_SPAWN` | Subscribe | Handle spawn request |
| `INPUT` | Subscribe | Process client input |
| `SERVER_PLAYER_DEAD` | Subscribe | Handle player death |
| `PLAY_SOUND` | Subscribe | Forward sound to local |
| `STOP_MUSIC` | Subscribe | Forward music stop |
| `LEVEL_CHANGE` | Subscribe | Handle level change |
| `PLAYER_ASSIGN` | Subscribe | Receive player assignment |
| `GAME_SCORE` | Subscribe | Receive score update |
| `ENTITY_HIT` | Subscribe | Handle hit effect |
| `ENTITY_POS` | Subscribe | Update entity position |
| `ENTITY_DESTROY` | Subscribe | Destroy entity |
| `ENEMY_DEAD` | Subscribe | Handle enemy death |
| `CLIENT_RESET` | Subscribe | Handle reset |

### `MenuSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `MousePressed` | Subscribe | Handle mouse click |
| `KeyPressed` | Subscribe | Handle key input |
| `MouseMoved` | Subscribe | Handle mouse movement |
| `PAUSE_GAME` | Subscribe | Show pause menu |
| `RESUME_GAME` | Subscribe | Hide pause menu |
| `WindowResized` | Subscribe | Handle window resize |
| `MusicPlay` | Send | Start background music |
| `MusicStop` | Send | Stop music |
| `ExitApplication` | Send | Exit game |
| `REQUEST_GAME_START` | Send (Network) | Request game start |
| `GAME_PAUSED` | Send | Notify pause |
| `GAME_RESUMED` | Send | Notify resume |

### `GameStateManager.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `REQUEST_GAME_STATE_CHANGE` | Subscribe | Handle state change |
| `GAME_STARTED` | Send | Notify game started |
| `GAME_RESET` | Send | Notify game reset |

### `CollisionSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `Collision` | Subscribe | Handle collision events |
| `PhysicCommand` | Send | Send physics commands |
| `SoundPlay` | Send | Play collision sounds |
| `PLAY_SOUND` | Broadcast | Sync sounds to clients |
| `ENTITY_HIT` | Broadcast | Notify hit effect |

### `LifeSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `ENEMY_DEAD` | Broadcast | Notify enemy death |
| `STOP_MUSIC` | Broadcast | Stop music on death |
| `PLAY_SOUND` | Broadcast | Play death sounds |
| `SERVER_PLAYER_DEAD` | Send | Notify player death |
| `ENTITY_DESTROY` | Broadcast | Destroy dead entity |
| `GAME_OVER` | Send | Signal game over |

### `EnemySystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `RESET_DIFFICULTY` | Subscribe | Reset difficulty |
| `ENTITY_DESTROY` | Broadcast | Destroy out-of-bounds |

### `PlayerSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `SoundPlay` | Send | Play shoot sounds |
| `PLAY_SOUND` | Broadcast | Sync sounds to clients |

### `RenderSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `RenderEntityCommand` | Send | Send render commands |

### `PhysicSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `PhysicCommand` | Send | Create/update bodies |
| `RenderEntityCommand` | Send | Sync transforms |

### `ParticleSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `RenderEntityCommand` | Send | Manage particles |

### `WinSystem.lua`
| Channel | Type | Description |
|---------|------|-------------|
| `LEVEL_CHANGE` | Broadcast | Notify level change |

---

## 🔗 See Also

- [Network Protocol](NETWORK_PROTOCOL.md)
- [Architecture Overview](ARCHITECTURE.md)
