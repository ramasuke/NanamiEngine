#pragma once
#include <memory>

#include "../glm/vec3.hpp"
#include "../glm/gtc/quaternion.hpp"
#include "Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace GamePlay::Spawn
{
    /**
     * プレハブを生成し、lifeTime_secs 秒後に破棄する(lifeTime_secs <= 0 なら時限破棄しない)。
     * 権威側(BehaviourTree アクション)と RPC 受信側の両方から同じ見た目を出すための共通処理。
     */
    std::weak_ptr<GameObject::IGameObject> SpawnPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        float lifeTime_secs);

    /**
     * プレハブを生成し、targetPos まで moveSpeed で直線移動させる。到達後 destroyOnFinish なら破棄する。
     */
    std::weak_ptr<GameObject::IGameObject> SpawnMovingPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& spawnPos,
        const glm::quat& rotation,
        const glm::vec3& targetPos,
        float moveSpeed,
        bool destroyOnFinish);
}
