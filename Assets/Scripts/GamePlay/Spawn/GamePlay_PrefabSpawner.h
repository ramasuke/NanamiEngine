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
     * プレハブを target の位置に生成し、どちらかが消えるまで target の位置へ付いて行かせる。
     * 親子にしないのは、プレハブの scale が target の scale で縮まないようにするため。破棄はプレハブ側(ParticleSystem の Destroy など)に任せる。
     */
    std::weak_ptr<GameObject::IGameObject> SpawnFollowingPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const std::shared_ptr<GameObject::IGameObject>& target);

    /**
     * プレハブを position に生成し、target に対するその位置を保ったまま付いて行かせる(target が傾けばそれに沿って回り込む)。
     * 向きは変えない。lifeTime_secs <= 0 なら時限破棄しない。
     */
    std::weak_ptr<GameObject::IGameObject> SpawnAttachedPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        const std::shared_ptr<GameObject::IGameObject>& target,
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
