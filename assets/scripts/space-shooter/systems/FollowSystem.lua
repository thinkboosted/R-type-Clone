local FollowSystem = {}

function FollowSystem.init()
    print("[FollowSystem] Initialized")
end

function FollowSystem.update(dt)
    local followers = ECS.getEntitiesWith({"Transform", "Follow"})
    for _, id in ipairs(followers) do
        local follow = ECS.getComponent(id, "Follow")
        local transform = ECS.getComponent(id, "Transform")

        local targetId = follow.targetId
        if targetId == "PLAYER" then
            targetId = NetworkSystem.myServerId
        end

        if targetId then
            local targetTransform = ECS.getComponent(targetId, "Transform")
            if targetTransform then
                transform.x = targetTransform.x + follow.offsetX
                transform.y = targetTransform.y + follow.offsetY
                transform.z = targetTransform.z + follow.offsetZ
            end
        end
    end
end

ECS.registerSystem(FollowSystem)
