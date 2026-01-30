-- [[ ROBUST BREAKOUT GAME LOOP ]] --

-- Constants
local PADDLE_SPEED = 30
local BALL_SPEED = 25
local BRICK_ROWS = 5
local BRICK_COLS = 13 -- Range -6 to 6
local BRICK_SPACING = 1.6
local WALL_DEPTH = 30

-- Game State
local Game = {
    paddle = {
        parts = {}, -- {left, center, right}
        x = 0
    },
    ball = nil,
    bricks = {}, -- List of {id, active}
    brickPool = {}, -- Pool of inactive brick IDs
    isGameOver = false,
    debugMode = false,
    debugDebounce = false
}

-- [[ HELPER FUNCTIONS ]] --

-- Generic Entity Creator
local function createObject(x, y, z, model, texture, mass, friction, dynamic, id_suffix)
    local id = ECS.createEntity({
        Transform = {x = x, y = y, z = z},
        Mesh = {modelPath = model, texturePath = texture},
        Collider = {type = "box", mass = mass, friction = friction}
    })

    -- SAFETY: Lock Z-axis and Rotation for 2D gameplay
    ECS.setConstraints(id, 1, 1, 0, true, true, true)

    -- FORCE SYNC: Send initial position to Renderer
    -- Optim: Use direct message to avoid table allocation overhead during init
    ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. "," .. x .. "," .. y .. "," .. z)
    ECS.sendMessage("RenderEntityCommand", "SetScale:" .. id .. ",1.0,1.0,1.0")

    return id
end

-- Wrapper for Bricks/Paddle (Cubes)
local function createCube(x, y, z, texture, mass, friction, dynamic, id_suffix)
    return createObject(x, y, z, "assets/models/cube.obj", texture, mass, friction, dynamic, id_suffix)
end

-- Create a Light Source
local function createSceneLight()
    -- Create Light Entity (ID: sun)
    -- Note: Engine requires "CreateEntity:LIGHT:id" via raw command
    local id = "sun"
    ECS.sendMessage("RenderEntityCommand", "CreateEntity:LIGHT:" .. id)
    ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. ",0,15,60") -- High Z for front lighting
    ECS.sendMessage("RenderEntityCommand", "SetLightProperties:" .. id .. ":1.0,1.0,1.0:0.8") -- Slightly dimmer to avoid blowout
    ECS.log("Light 'sun' created.")
end

-- Create the Camera
local function createSceneCamera()
    local id = ECS.createEntity({
        Transform = {x = 0, y = 0, z = 70},
        Camera = {}
    })
    -- FORCE SYNC: Camera creation doesn't auto-send Transform
    ECS.sendMessage("RenderEntityCommand", "SetPosition:" .. id .. ",0,0,70")
    ECS.sendMessage("RenderEntityCommand", "SetRotation:" .. id .. ",0,0,0")
    ECS.log("Camera created: " .. id)
    return id
end

-- [[ GAME LOGIC ]] --

function Game:init()
    ECS.log("Initializing Robust Breakout...")

    -- 1. Setup Scene
    createSceneCamera()
    createSceneLight()

    -- 2. Create Paddle (Composite: Left, Center, Right)
    -- Why? Engine doesn't scale meshes visually. We simulate a wide paddle with 3 blocks.
    local py = -12
    local p1 = createCube(-1.1, py, 0, "assets/textures/test.png", 50, 0, true, "p_left")
    local p2 = createCube(0, py, 0, "assets/textures/test.png", 50, 0, true, "p_center")
    local p3 = createCube(1.1, py, 0, "assets/textures/test.png", 50, 0, true, "p_right")

    self.paddle.parts = {p1, p2, p3}
    self.paddle.x = 0

    -- 3. Create Ball (Sphere)
    -- We use assets/models/sphere.obj and a valid texture (or empty string/test.png)
    self.ball = createObject(0, 0, 0, "assets/models/sphere.obj", "assets/textures/test.png", 1, 0, true, "ball")

    -- Set Initial Velocity
    ECS.setLinearVelocity(self.ball, 5, 10, 0)

    -- 4. Create Walls (Composite)
    -- Top Wall
    for i = -8, 8 do createCube(i * 1.5, 14, 0, "assets/textures/test.png", 0, 0, false, "wall_top") end
    -- Left Wall
    for i = -12, 14 do createCube(-12, i, 0, "assets/textures/test.png", 0, 0, false, "wall_left") end
    -- Right Wall
    for i = -12, 14 do createCube(12, i, 0, "assets/textures/test.png", 0, 0, false, "wall_right") end

    -- 5. Create Bricks with Pooling
    local startY = 2
    for row = 0, BRICK_ROWS - 1 do
        for col = 0, BRICK_COLS - 1 do
            local bx = (col - (BRICK_COLS/2)) * BRICK_SPACING
            local by = startY + (row * 1.5)
            local bId = createCube(bx, by, 0, "assets/textures/test.png", 0, 0, false, "brick")
            table.insert(self.bricks, {id = bId, active = true, x = bx, y = by})
        end
    end

    ECS.log("Game Initialized.")
end

function Game:update(dt)
    if self.isGameOver then return end

    -- 1. Paddle Movement
    local vx = 0
    if ECS.isKeyPressed("Q") then vx = -PADDLE_SPEED end
    if ECS.isKeyPressed("D") then vx = PADDLE_SPEED end

    -- Debug Toggle (Key 'P')
    if ECS.isKeyPressed("P") then
        if not self.debugDebounce then
            self.debugMode = not self.debugMode
            ECS.sendMessage("PhysicCommand", "SetDebugMode:" .. (self.debugMode and 1 or 0))
            ECS.log("Debug Mode: " .. (self.debugMode and "ON" or "OFF"))
            self.debugDebounce = true
        end
    else
        self.debugDebounce = false
    end

    -- Apply Velocity to ALL paddle parts
    for _, pid in ipairs(self.paddle.parts) do
        ECS.setLinearVelocity(pid, vx, 0, 0)
    end

    -- 2. Paddle Glues (Keep parts together)
    -- The physics engine might separate them on collision. We force them back relative to Center.
    local centerId = self.paddle.parts[2]
    local leftId = self.paddle.parts[1]
    local rightId = self.paddle.parts[3]

    -- We need the Center's position.
    -- Since we receive 'onEntityUpdated', we could cache it, but 'getComponent' is safer for immediate check.
    local cT = ECS.getComponent(centerId, "Transform")
    if cT then
        local cx, cy = cT.x, cT.y

        -- Force Left/Right positions relative to Center (Glue)
        -- Using updateComponent triggers automatic sync to Physics and Renderer

        -- Left (-1.1)
        ECS.updateComponent(leftId, "Transform", {x = cx - 1.1, y = cy, z = 0})

        -- Right (+1.1)
        ECS.updateComponent(rightId, "Transform", {x = cx + 1.1, y = cy, z = 0})
    end

    -- 3. Ball Reset (Death)
    local ballT = ECS.getComponent(self.ball, "Transform")
    if ballT and ballT.y < -15 then
        -- Reset Ball
        -- Update Component -> Triggers Engine Sync (Render+Physic)
        ECS.updateComponent(self.ball, "Transform", {x=0, y=0, z=0})
        ECS.setLinearVelocity(self.ball, 0, 0, 0)

        -- Launch after short delay (simulated by just waiting for input or immediate)
        ECS.setLinearVelocity(self.ball, 5 * (math.random() > 0.5 and 1 or -1), 10, 0)
    end

    -- 4. Manual Collision Handling (Bricks)
    -- Physics engine handles bounce, but we need to Detect & 'Destroy' bricks.
    -- Since we lack a reliable 'onCollision' callback with IDs in this binding version,
    -- We use a simpler distance check for the ball against active bricks.
    -- (Optim: Only check if ball is in brick area Y > 0)
    if ballT and ballT.y > 0 then
        for i, brick in ipairs(self.bricks) do
            if brick.active then
                -- Simple AABB/Sphere check
                local dx = math.abs(ballT.x - brick.x)
                local dy = math.abs(ballT.y - brick.y)

                if dx < 1.0 and dy < 1.0 then
                    -- Hit!
                    brick.active = false

                    -- "Destroy" by teleporting away (Pooling)
                    -- Update Component -> Triggers Engine Sync (Render+Physic)
                    ECS.updateComponent(brick.id, "Transform", {x=0, y=500, z=0})
                    ECS.setLinearVelocity(brick.id, 0, 0, 0)

                    -- Optional: Bounce ball manually if needed, but physics usually handles strict collision
                    -- But since we teleported the brick, the bounce might be cut short.
                    -- Force a generic bounce effect?
                    -- ECS.setLinearVelocity(self.ball, ballT.vx, -ballT.vy, 0) -- hypothetical
                    break -- Handle one brick/frame to prevent tunneling issues
                end
            end
        end
    end
end

-- Callbacks
function Game:onEntityUpdated(id, x, y, z, rx, ry, rz)
    -- Sync Physics -> Renderer is now handled by C++ Engine (LuaECSManager.cpp)
    -- We only update the internal Lua component for logic access (Game.paddle.parts, etc.)
    ECS.updateComponent(id, "Transform", {x=x, y=y, z=z, rx=rx, ry=ry, rz=rz})
end

-- Bind to Global Scope for Engine calls
Breakout = {}
function Breakout.init() Game:init() end
function Breakout.update(dt) Game:update(dt) end
function Breakout.onEntityUpdated(id, x, y, z, rx, ry, rz) Game:onEntityUpdated(id, x, y, z, rx, ry, rz) end

ECS.registerSystem(Breakout)
