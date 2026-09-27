#include "GamePlay_PrefabSpawner.h"

#include "Engine/Core/Coroutine/Coroutine.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "Engine/Core/Coroutine/Awaitable/Yield/Coroutine_WaitYield.h"
#include "Engine/Module/Component/BoneSync/BoneSync.h"
#include "Engine/Module/GameObject/Interface/IGameObject.h"
#include "Engine/Module/GameObject/Transform/Transform.h"
#include "Engine/Module/Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "Libs/LibCore/Tween/Ease/Ease.h"
#include "../../Core/Network/Rpc/Custom_RpcType.h"
#include "../Network/GamePlay_NetworkObjectIdOf.h"

namespace GamePlay::Spawn
{
    namespace
    {
        Coroutine::Task<void> DestroyAfterTimeAsync(
            std::weak_ptr<GameObject::IGameObject> gameObject,
            const float lifeTime_secs)
        {
            co_await Coroutine::WaitForSeconds(lifeTime_secs);

            const auto object = gameObject.lock();
            if (!object)
                co_return;

            object->OnDestroy();
        }

        Coroutine::Task<void> FollowAsync(
            std::weak_ptr<GameObject::IGameObject> gameObject,
            std::weak_ptr<GameObject::IGameObject> target)
        {
            while (true)
            {
                co_await Coroutine::WaitYield();

                const auto object = gameObject.lock();
                const auto followed = target.lock();
                if (!object || !followed)
                    co_return;

                object->Transform().SetWorldPos(followed->Transform().GetWorldPos());
            }
        }

        glm::vec3 BoneWorldPos(GameObject::IGameObject& owner, const std::string& boneName, const glm::vec3& localOffset)
        {
            const auto boneSync = owner.Components().Catch<NanamiEngine::Module::Component::BoneSync>().lock();
            if (boneSync)
            {
                if (const auto boneMatrix = boneSync->GetBoneWorldMatrix(boneSync->FindBoneIndex(boneName)))
                    return glm::vec3(*boneMatrix * glm::vec4(localOffset, 1.0f));
            }
            return owner.Transform().GetWorldPos();
        }

        Coroutine::Task<void> FollowBoneAsync(
            std::weak_ptr<GameObject::IGameObject> gameObject,
            std::weak_ptr<GameObject::IGameObject> owner,
            const std::string boneName,
            const glm::vec3 localOffset)
        {
            while (true)
            {
                co_await Coroutine::WaitYield();

                const auto object   = gameObject.lock();
                const auto followed = owner.lock();
                if (!object || !followed)
                    co_return;

                object->Transform().SetWorldPos(BoneWorldPos(*followed, boneName, localOffset));
            }
        }

        Coroutine::Task<void> AttachAsync(
            std::weak_ptr<GameObject::IGameObject> gameObject,
            std::weak_ptr<GameObject::IGameObject> target)
        {
            glm::vec3 localPos{};
            {
                const auto object   = gameObject.lock();
                const auto attached = target.lock();
                if (!object || !attached)
                    co_return;

                const auto& targetTransform = attached->Transform();
                localPos = glm::inverse(targetTransform.GetWorldRot())
                         * (object->Transform().GetWorldPos() - targetTransform.GetWorldPos());
            }

            while (true)
            {
                co_await Coroutine::WaitYield();

                const auto object   = gameObject.lock();
                const auto attached = target.lock();
                if (!object || !attached)
                    co_return;

                const auto& targetTransform = attached->Transform();
                object->Transform().SetWorldPos(targetTransform.GetWorldPos() + targetTransform.GetWorldRot() * localPos);
            }
        }

        Coroutine::Task<void> MoveAsync(
            std::weak_ptr<GameObject::IGameObject> gameObject,
            const glm::vec3 targetPos,
            const float moveSpeed,
            const bool destroyOnFinish)
        {
            const auto object = gameObject.lock();
            if (!object)
                co_return;

            auto& transform = object->Transform();
            const glm::vec3 startPos = transform.GetWorldPos();

            const float distance    = glm::distance(startPos, targetPos);
            const float durationSec = distance / moveSpeed;

            const auto tween = tweeny::from(startPos)
                .to(targetPos)
                .during(static_cast<int>(durationSec * 1000.0f))
                .via(Tween::Ease(EaseType::Linear));

            co_await Coroutine::WaitForTween(transform, tween);

            if (!gameObject.expired() && destroyOnFinish)
                gameObject.lock()->OnDestroy();
        }
    }

    std::weak_ptr<GameObject::IGameObject> SpawnPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        const float lifeTime_secs)
    {
        const auto spawned = Scene::GameObject::Instantiate(prefab, position);

        if (lifeTime_secs > 0.0f)
            Coroutine::StartCoroutine(DestroyAfterTimeAsync(spawned, lifeTime_secs));

        return spawned;
    }

    std::weak_ptr<GameObject::IGameObject> SpawnAttachedPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        const std::shared_ptr<GameObject::IGameObject>& target,
        const float lifeTime_secs)
    {
        const auto spawned = SpawnPrefab(prefab, position, lifeTime_secs);
        if (target)
            Coroutine::StartCoroutine(AttachAsync(spawned, target));

        return spawned;
    }

    std::weak_ptr<GameObject::IGameObject> SpawnFollowingPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const std::shared_ptr<GameObject::IGameObject>& target)
    {
        if (!target)
            return {};

        const auto spawned = Scene::GameObject::Instantiate(prefab, target->Transform().GetWorldPos());
        Coroutine::StartCoroutine(FollowAsync(spawned, target));
        return spawned;
    }

    std::weak_ptr<GameObject::IGameObject> SpawnBoneFollowingPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const std::shared_ptr<GameObject::IGameObject>& owner,
        const std::string& boneName,
        const glm::vec3& localOffset)
    {
        if (!owner)
            return {};

        const auto spawned = Scene::GameObject::Instantiate(prefab, BoneWorldPos(*owner, boneName, localOffset));
        Coroutine::StartCoroutine(FollowBoneAsync(spawned, owner, boneName, localOffset));
        return spawned;
    }

    std::weak_ptr<GameObject::IGameObject> SpawnMovingPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& spawnPos,
        const glm::quat& rotation,
        const glm::vec3& targetPos,
        const float moveSpeed,
        const bool destroyOnFinish)
    {
        const auto spawned = Scene::GameObject::Instantiate(prefab, spawnPos, rotation);

        Coroutine::StartCoroutine(MoveAsync(spawned, targetPos, moveSpeed, destroyOnFinish));

        return spawned;
    }

    std::weak_ptr<GameObject::IGameObject> SpawnOrientedPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        const std::optional<glm::quat>& rotation,
        const std::optional<float> scale)
    {
        const auto spawned = rotation
            ? Scene::GameObject::Instantiate(prefab, position, *rotation)
            : Scene::GameObject::Instantiate(prefab, position);

        if (scale)
        {
            if (const auto object = spawned.lock())
                object->Transform().SetLocalScale(glm::vec3(*scale));
        }
        return spawned;
    }

    std::weak_ptr<GameObject::IGameObject> SpawnOrientedPrefabSynced(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        const std::optional<glm::quat>& rotation,
        const std::optional<float> scale,
        GameObject::IGameObject& sender)
    {
        const auto spawned = SpawnOrientedPrefab(prefab, position, rotation, scale);

        const auto senderId = Network::NetworkObjectIdOf(sender);
        if (senderId != Core::Network::NetworkObjectId::Invalid())
            GameCore::Network::SpawnOrientedPrefabRpc::Send(senderId, Core::Network::DeliveryMode::Reliable, prefab.GetGuid(), position, rotation, scale);

        return spawned;
    }
}
