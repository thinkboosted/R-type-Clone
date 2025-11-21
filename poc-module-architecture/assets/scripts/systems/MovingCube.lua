local MovingCube = {}

local cubeId = nil
local lightId = nil
local cameraId = nil
local time = 0

function MovingCube.init()
    print("[Lua] Initializing MovingCube System")
    ²
    cubeId = Scene.createEntity("MESH")
    print("[Lua] Created Cube with ID: " .. tostring(cubeId))
    Scene.setPosition(cubeId, 0, 0, 0)

    cameraId = Scene.createEntity("CAMERA")
    print("[Lua] Created Camera with ID: " .. tostring(cameraId))
    Scene.setPosition(cameraId, 0, 0, 5)
    Scene.setActiveCamera(cameraId)

    lightId = Scene.createEntity("LIGHT")
    print("[Lua] Created Light with ID: " .. tostring(lightId))
    Scene.setPosition(lightId, 2, 2, 2)
    Scene.setLightProperties(lightId, 1.0, 1.0, 1.0, 1.0)
end

function MovingCube.update(dt)
    if cubeId then
        time = time + dt
        -- Rotate the cube
        Scene.rotateEntity(cubeId, 0.01, 0.02, 0)
    end
end

return MovingCube
