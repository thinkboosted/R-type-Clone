local CameraSystem = {}

function CameraSystem.init()
end

function CameraSystem.update(dt)
    local players = ECS.getEntitiesWith({"Player", "Transform"})
    local cameras = ECS.getEntitiesWith({"Camera", "Transform"})

    if #players > 0 and #cameras > 0 then
        local playerID = players[1]
        local cameraID = cameras[1]

        local pTrans = ECS.getComponent(playerID, "Transform")
        local cTrans = ECS.getComponent(cameraID, "Transform")

        local dist = 10.0
        local height = 5.0

        local ry = pTrans.ry

        local offsetX = -math.sin(ry) * dist
        local offsetZ = -math.cos(ry) * dist

        local targetX = pTrans.x + offsetX
        local targetY = pTrans.y + height
        local targetZ = pTrans.z + offsetZ

        cTrans.x = targetX
        cTrans.y = targetY
        cTrans.z = targetZ

        cTrans.ry = ry + math.pi

        cTrans.rx = -0.3
    end
end

ECS.registerSystem(CameraSystem)
