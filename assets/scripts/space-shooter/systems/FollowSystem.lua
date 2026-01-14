local FollowSystem = {}

function FollowSystem.init()
    print("[FollowSystem] Initialized")
end

function FollowSystem.update(dt)
    if ECS.isPaused then return end

    local followers = ECS.getEntitiesWith({"Transform", "Follow"})
    for _, id in ipairs(followers) do
        local follow = ECS.getComponent(id, "Follow")
        local transform = ECS.getComponent(id, "Transform")

        local targetTransform = ECS.getComponent(follow.targetId, "Transform")

        if targetTransform then
            transform.x = targetTransform.x + follow.offsetX
            transform.y = targetTransform.y + follow.offsetY
            transform.z = targetTransform.z + follow.offsetZ
        end
    end
end

ECS.registerSystem(FollowSystem)
