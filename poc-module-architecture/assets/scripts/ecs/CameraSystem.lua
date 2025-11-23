local CameraSystem = {}

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

        -- Third person offset
        local offsetX = 0
        local offsetY = 5
        local offsetZ = -10

        -- Simple follow
        cTrans.x = pTrans.x + offsetX
        cTrans.y = pTrans.y + offsetY
        cTrans.z = pTrans.z + offsetZ
    end
end

ECS.registerSystem(CameraSystem)
