-- Camera System for 3D Platformer
-- Follows the player from behind

CameraSystem = {}

function CameraSystem.init()
    print("[CameraSystem] Initialized")
end

function CameraSystem.update(dt)
    local players = ECS.getEntitiesWith({"Player", "Transform"})
    local cameras = ECS.getEntitiesWith({"Camera", "Transform"})

    if #players > 0 and #cameras > 0 then
        local playerID = players[1]
        local cameraID = cameras[1]

        local pTrans = ECS.getComponent(playerID, "Transform")
        local cTrans = ECS.getComponent(cameraID, "Transform")

        -- Camera follows behind the player
        local dist = 12.0
        local height = 8.0

        local ry = pTrans.ry

        -- Calculate camera position behind player
        local offsetX = -math.sin(ry) * dist
        local offsetZ = -math.cos(ry) * dist

        local targetX = pTrans.x + offsetX
        local targetY = pTrans.y + height
        local targetZ = pTrans.z + offsetZ

        -- Smooth camera follow
        local smoothness = 5.0 * dt
        cTrans.x = cTrans.x + (targetX - cTrans.x) * smoothness
        cTrans.y = cTrans.y + (targetY - cTrans.y) * smoothness
        cTrans.z = cTrans.z + (targetZ - cTrans.z) * smoothness

        -- Camera looks at player
        cTrans.ry = ry + math.pi
        cTrans.rx = -0.4
    end
end

ECS.registerSystem(CameraSystem)
