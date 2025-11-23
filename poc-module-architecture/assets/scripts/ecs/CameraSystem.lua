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
        local player = ECS.getComponent(playerID, "Player")

        -- Update camera pitch based on input
        if player.inputY ~= 0 then
            cTrans.rx = cTrans.rx + (player.inputY * dt * 2.0)
            -- Clamp pitch
            if cTrans.rx > 1.0 then cTrans.rx = 1.0 end
            if cTrans.rx < -1.0 then cTrans.rx = -1.0 end
        end

        -- Third person offset calculation
        local dist = 10.0
        local height = 5.0

        -- Calculate camera position behind player
        -- We use player's Y rotation (Yaw) to position camera behind
        local ry = pTrans.ry

        local offsetX = -math.sin(ry) * dist
        local offsetZ = -math.cos(ry) * dist

        -- Smooth follow (simple lerp)
        local lerpSpeed = 5.0 * dt

        local targetX = pTrans.x + offsetX
        local targetY = pTrans.y + height
        local targetZ = pTrans.z + offsetZ

        cTrans.x = cTrans.x + (targetX - cTrans.x) * lerpSpeed
        cTrans.y = cTrans.y + (targetY - cTrans.y) * lerpSpeed
        cTrans.z = cTrans.z + (targetZ - cTrans.z) * lerpSpeed

        -- Make camera look at player
        -- Yaw: same as player
        cTrans.ry = ry -- Look forward (same direction as player)

        -- Pitch is controlled by user (cTrans.rx)
    end
end

ECS.registerSystem(CameraSystem)
