#pragma once
#include <memory>
#include <optional>
#include <string>

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
    // NOTE: プレハブを生成し、lifeTime_secs 秒後に破棄する(<= 0 なら破棄しない)
    std::weak_ptr<GameObject::IGameObject> SpawnPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        float lifeTime_secs);

    // NOTE: プレハブを target の位置に生成して追従させる。破棄はプレハブ側に任せる
    // NOTE: target の scale を受けないよう親子にはしない
    std::weak_ptr<GameObject::IGameObject> SpawnFollowingPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const std::shared_ptr<GameObject::IGameObject>& target);

    // NOTE: プレハブを owner のボーン boneName に追従させる。破棄はプレハブ側に任せる
    // NOTE: BoneSync かボーンが無ければ owner の位置に追従する
    std::weak_ptr<GameObject::IGameObject> SpawnBoneFollowingPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const std::shared_ptr<GameObject::IGameObject>& owner,
        const std::string& boneName,
        const glm::vec3& localOffset);

    // NOTE: プレハブを position に生成し、target からの相対位置を保って追従させる(向きは変えない)
    std::weak_ptr<GameObject::IGameObject> SpawnAttachedPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        const std::shared_ptr<GameObject::IGameObject>& target,
        float lifeTime_secs);

    // NOTE: プレハブを生成し、targetPos まで moveSpeed で直線移動させる。到達後 destroyOnFinish なら破棄する
    std::weak_ptr<GameObject::IGameObject> SpawnMovingPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& spawnPos,
        const glm::quat& rotation,
        const glm::vec3& targetPos,
        float moveSpeed,
        bool destroyOnFinish);

    // NOTE: プレハブを position に生成する。rotation / scale が無ければプレハブのまま
    std::weak_ptr<GameObject::IGameObject> SpawnOrientedPrefab(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        const std::optional<glm::quat>& rotation,
        std::optional<float> scale);

    // NOTE: SpawnOrientedPrefab し、オンラインなら sender の NetworkGameObject 宛てに他のピアへも出させる
    std::weak_ptr<GameObject::IGameObject> SpawnOrientedPrefabSynced(
        Asset::PrefabGameObjectFile& prefab,
        const glm::vec3& position,
        const std::optional<glm::quat>& rotation,
        std::optional<float> scale,
        GameObject::IGameObject& sender);
}
