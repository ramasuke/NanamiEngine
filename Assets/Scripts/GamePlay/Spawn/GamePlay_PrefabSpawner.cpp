#include "GamePlay_PrefabSpawner.h"

#include "../../../../Engine/Core/Coroutine/Coroutine.h"
#include "../../../../Engine/Core/Coroutine/Awaitable/WaitForSeconds/Coroutine_WaitForSeconds.h"
#include "../../../../Engine/Core/Coroutine/Awaitable/WaitForTween/Coroutine_WaitForTween.h"
#include "../../../../Engine/Module/GameObject/Interface/IGameObject.h"
#include "../../../../Engine/Module/GameObject/Transform/Transform.h"
#include "../../../../Engine/Module/Scene/GameObject/Helper/GameObject.h"
#include "../../../../Libs/LibCore/Tween/Ease/Ease.h"

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
}
